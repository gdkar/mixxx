#include "sources/soundsourceffmpeg.h"

#include <vector>

#define AUDIOSOURCEFFMPEG_CACHESIZE 1000
#define AUDIOSOURCEFFMPEG_POSDISTANCE ((1024 * 1000) / 8)

namespace Mixxx {

QStringList SoundSourceProviderFFmpeg::getSupportedFileExtensions() const {
    QStringList list;
    AVInputFormat *l_SInputFmt  = nullptr;
    while ((l_SInputFmt = av_iformat_next(l_SInputFmt)) && l_SInputFmt->name) {
      auto fmts = QString(l_SInputFmt->name).split(",");
      for(auto fmt: fmts ){
        if(!list.contains(fmt))
          list.append(fmt);
      }
    }
    return list;
}

SoundSourceFFmpeg::SoundSourceFFmpeg(QUrl url)
    : SoundSource(url),
      m_pFormatCtx(avformat_alloc_context()),
      m_iAudioStream(-1),
      m_pCodecCtx(nullptr),
      m_pCodec(nullptr),
      m_pSwrCtx(swr_alloc()),
      m_currentMixxxFrameIndex(0){
}
SoundSourceFFmpeg::~SoundSourceFFmpeg() {
  close();
  av_freep(&m_pFormatCtx);
  swr_free(&m_pSwrCtx);
}
Result SoundSourceFFmpeg::tryOpen(const AudioSourceConfig& /*audioSrcCfg*/) {
    unsigned int i;
    AVDictionary *l_iFormatOpts = NULL;
    const QByteArray qBAFilename(getLocalFileNameBytes());
    qDebug() << "New SoundSourceFFmpeg :" << qBAFilename;
    if(!m_pFormatCtx)m_pFormatCtx=avformat_alloc_context();
#if LIBAVCODEC_VERSION_INT < 3622144
    m_pFormatCtx->max_analyze_duration2 = 999999999;
#endif
    // Open file and make m_pFormatCtx
    if (avformat_open_input(&m_pFormatCtx, qBAFilename.constData(), NULL, &l_iFormatOpts)!=0) {
        qDebug() << "av_open_input_file: cannot open" << qBAFilename;
        return ERR;
    }
#if LIBAVCODEC_VERSION_INT > 3544932
    av_dict_free(&l_iFormatOpts);
#endif
    // Retrieve stream information
    if (avformat_find_stream_info(m_pFormatCtx, NULL)<0) {
        qDebug() << "av_find_stream_info: cannot open" << qBAFilename;
        return ERR;
    }

    //debug only (Enable if needed)
    //av_dump_format(m_pFormatCtx, 0, qBAFilename.constData(), false);

    // Find the first audio stream
    m_iAudioStream=-1;
    for (i=0; i<m_pFormatCtx->nb_streams; i++)
        if (m_pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
            m_iAudioStream=i;
            break;
        }
    if (m_iAudioStream == -1) {
        qDebug() << "ffmpeg: cannot find an audio stream: cannot open"
                 << qBAFilename;
        return ERR;
    }
    // Get a pointer to the codec context for the audio stream
    m_pCodecCtx=m_pFormatCtx->streams[m_iAudioStream]->codec;
    // Find the decoder for the audio stream
    if (!(m_pCodec=avcodec_find_decoder(m_pCodecCtx->codec_id))) {
        qDebug() << "ffmpeg: cannot find a decoder for" << qBAFilename;
        return ERR;
    }
    m_pCodecCtx->refcounted_frames=1;
    if (avcodec_open2(m_pCodecCtx, m_pCodec, nullptr)<0) {
        qDebug() << "ffmpeg:  cannot open" << qBAFilename;
        return ERR;
    }
    m_pSwrCtx=swr_alloc_set_opts(m_pSwrCtx, 
        av_get_default_channel_layout(m_pCodecCtx->channels),
        m_pCodecCtx->sample_fmt,
        m_pCodecCtx->sample_rate,
        AV_CH_LAYOUT_STEREO,
        AV_SAMPLE_FMT_FLT,
        m_pCodecCtx->sample_rate,
        0,nullptr);
    if(!m_pSwrCtx||swr_init(m_pSwrCtx)<0)return ERR;
    setChannelCount(2);
    setFrameRate(m_pCodecCtx->sample_rate);
    setFrameCount((m_pFormatCtx->duration * m_pCodecCtx->sample_rate) / AV_TIME_BASE);
    qDebug() << "ffmpeg: Samplerate: " << getFrameRate() << ", Channels: " << getChannelCount() << "\n";
    return OK;
}
void SoundSourceFFmpeg::close() {
    avcodec_close(m_pCodecCtx);
    avformat_close_input(&m_pFormatCtx);
    for(auto &frame : m_frameCache)
    {
      av_frame_free(&frame);
    }
    m_frameCache.clear();
}
void SoundSourceFFmpeg::readFramesToCache(unsigned int count, SINT offset) {
    int ret = 0,got_frame;
    AVPacket l_SPacket, l_FPacket;
    l_SPacket.data = nullptr;
    l_SPacket.size = 0;
    av_init_packet(&l_SPacket);
    l_FPacket = l_SPacket;
    AVFrame *l_pFrame   = av_frame_alloc();
    AVFrame *l_swrFrame;
    AVRational time_base = {1,(int)getFrameRate()};
    off_t pkt_pts = AV_NOPTS_VALUE;
    if(offset < m_cacheStart || ! m_frameCache.size() || offset > m_cacheEnd + m_frameCache.back()->nb_samples)
    {
      for(auto &frame : m_frameCache)
        av_frame_free(&frame);
      m_frameCache.clear();
      m_cacheStart = AV_NOPTS_VALUE;
      m_cacheEnd   = AV_NOPTS_VALUE;
      off_t seek_pts = offset * AV_TIME_BASE * 1.0/m_pCodecCtx->sample_rate;
      av_seek_frame(m_pFormatCtx,-1,seek_pts,AVSEEK_FLAG_BACKWARD);
      avcodec_flush_buffers(m_pCodecCtx);
    }
    do{
      ret = av_read_frame(m_pFormatCtx,&l_FPacket);
      if(ret<0){
        av_free_packet(&l_FPacket);
        l_FPacket.data = nullptr;
        l_FPacket.size = 0;
        av_init_packet(&l_FPacket);
        l_SPacket = l_FPacket;
      }else{
        l_SPacket = l_FPacket;
      }
      if(pkt_pts == AV_NOPTS_VALUE)
      {
        pkt_pts = av_rescale_q(l_SPacket.pts,m_pFormatCtx->streams[m_iAudioStream]->time_base,AV_TIME_BASE_Q);
      }
      if(m_cacheStart==AV_NOPTS_VALUE) {
        m_cacheStart = pkt_pts;
        m_cacheEnd   = pkt_pts;
      }
      while(l_SPacket.size>0){
        ret = avcodec_decode_audio4(m_pCodecCtx, l_pFrame,&got_frame,&l_SPacket);
        if(ret<0){
          av_free_packet(&l_FPacket);
          break;
        }
        if(ret==0)ret = l_SPacket.size;
        l_SPacket.data += ret;
        l_SPacket.size -= ret;
        if(!l_SPacket.size){
          av_free_packet(&l_FPacket);
          l_FPacket.data = nullptr;
          l_FPacket.size = 0;
          av_init_packet(&l_FPacket);
          l_SPacket=l_FPacket;
        }
        if(!got_frame)break;
        l_pFrame->pts = pkt_pts;
        pkt_pts   += l_pFrame->nb_samples;
        m_cacheEnd = pkt_pts;
        l_swrFrame = av_frame_alloc();
        l_swrFrame->channels = l_pFrame->channels;
        l_swrFrame->channel_layout   = l_pFrame->channel_layout;
        l_swrFrame->format           = AV_SAMPLE_FMT_FLT;
        l_swrFrame->sample_rate      = l_pFrame->sample_rate;
        l_swrFrame->nb_samples       = 0;
        swr_config_frame(m_pSwrCtx,l_swrFrame,l_pFrame);
        swr_convert_frame(m_pSwrCtx,l_swrFrame,l_pFrame);
        l_swrFrame->pts              = l_pFrame->pts;
        m_frameCache.push_back(l_swrFrame);
        l_swrFrame=nullptr;
      }
    }while(m_cacheEnd==AV_NOPTS_VALUE || (m_cacheStart!=AV_NOPTS_VALUE && m_cacheStart<=offset && m_cacheEnd < offset + count ));
    av_frame_free(&l_pFrame);
}

SINT SoundSourceFFmpeg::seekSampleFrame(SINT frameIndex) {
    DEBUG_ASSERT(isValidFrameIndex(frameIndex));
    int ret = 0;
    qint64 i = 0;
    readFramesToCache(1024,frameIndex);
    if(m_cacheStart>frameIndex)     frameIndex = m_cacheStart;
    if(m_cacheEnd<frameIndex) frameIndex = m_cacheEnd;
    m_currentMixxxFrameIndex                   = frameIndex;
    return frameIndex;
}
SINT SoundSourceFFmpeg::readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer) {
   readFramesToCache(numberOfFrames,m_currentMixxxFrameIndex); 
   int findex = 0;
   SINT orig_frames = numberOfFrames;
   while(findex <  m_frameCache.size() && (m_frameCache[findex])->pts < m_currentMixxxFrameIndex)
     findex++;
    while(numberOfFrames && findex<m_frameCache.size())
    {
      auto frame = m_frameCache[findex];
      off_t skip = m_currentMixxxFrameIndex-(frame)->pts;
      off_t here = (frame)->nb_samples - skip;
      off_t copy = std::min(here,numberOfFrames);
      memmove(sampleBuffer,(frame)->extended_data[0] + (2*sizeof(CSAMPLE)*skip),2*sizeof(CSAMPLE)*copy);
      numberOfFrames -= copy;
      sampleBuffer   += copy * 2;
      m_currentMixxxFrameIndex += copy;
      if(copy == here){findex++;}
      else{break;}
    }
    while(m_frameCache.size()>1 && m_frameCache[1]->pts <= m_currentMixxxFrameIndex)
    {
      av_frame_free(&m_frameCache[0]);
      m_frameCache.pop_front();
    }
    return orig_frames - numberOfFrames;
}
} // namespace Mixxx
