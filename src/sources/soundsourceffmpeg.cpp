#include "sources/soundsourceffmpeg.h"

#include <vector>

#define AUDIOSOURCEFFMPEG_CACHESIZE 1000
#define AUDIOSOURCEFFMPEG_POSDISTANCE ((1024 * 1000) / 8)

namespace Mixxx {

QStringList SoundSourceProviderFFmpeg::getSupportedFileExtensions() const {
    QStringList list;
    AVInputFormat *l_SInputFmt  = nullptr;

    while ((l_SInputFmt = av_iformat_next(l_SInputFmt))) {
        if (l_SInputFmt->name == nullptr) {break;}
        else{
          for(const auto &str : QString(l_SInputFmt->name).split(".")){
            if(!list.contains(str))list.append(str);
          }
        }
    }
    return list;
}

SoundSourceFFmpeg::SoundSourceFFmpeg(QUrl url)
    : SoundSource(url),
      m_pFormatCtx(nullptr),
      m_iAudioStream(-1),
      m_pCodecCtx(nullptr),
      m_pCodec(nullptr),
      m_pResample(nullptr),
      m_currentMixxxFrameIndex(0),
      m_bIsSeeked(false),
      m_lCacheFramePos(0),
      m_lCacheStartFrame(0),
      m_lCacheEndFrame(0),
      m_lCacheLastPos(0){
}

SoundSourceFFmpeg::~SoundSourceFFmpeg() {
    close();
}

Result SoundSourceFFmpeg::tryOpen(const AudioSourceConfig& /*audioSrcCfg*/) {
    unsigned int i;
    AVDictionary *l_iFormatOpts = nullptr;
    qDebug() << "New SoundSourceFFmpeg :" << getLocalFileName();
    DEBUG_ASSERT(!m_pFormatCtx);
    m_pFormatCtx = avformat_alloc_context();
#if LIBAVCODEC_VERSION_INT < 3622144
    m_pFormatCtx->max_analyze_duration = 999999999;
#endif
    // Open file and make m_pFormatCtx
    if (avformat_open_input(&m_pFormatCtx, getLocalFileName().toLocal8Bit().constData(), nullptr,&l_iFormatOpts)!=0) {
        qDebug() << "av_open_input_file: cannot open" << getLocalFileName();
        return ERR;
    }
#if LIBAVCODEC_VERSION_INT > 3544932
    av_dict_free(&l_iFormatOpts);
#endif
    // Retrieve stream information
    if (avformat_find_stream_info(m_pFormatCtx, nullptr)<0) {
        qDebug() << "av_find_stream_info: cannot open" << getLocalFileName();
        return ERR;
    }
    //debug only (Enable if needed)
    //av_dump_format(m_pFormatCtx, 0, qBAFilename.constData(), false);
    // Find the first audio stream
    m_iAudioStream=-1;
    for (i=0; i<m_pFormatCtx->nb_streams; i++){
        if (m_pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
            m_iAudioStream=i;
            break;
        }
    }
    if (m_iAudioStream == -1) {
        qDebug() << "ffmpeg: cannot find an audio stream: cannot open"
                 << getLocalFileName();
        return ERR;
    }
    // Get a pointer to the codec context for the audio stream
    m_pCodecCtx=m_pFormatCtx->streams[m_iAudioStream]->codec;
    // Find the decoder for the audio stream
    if (!(m_pCodec=avcodec_find_decoder(m_pCodecCtx->codec_id))) {
        qDebug() << "ffmpeg: cannot find a decoder for" << getLocalFileName();
        return ERR;
    }
    if (avcodec_open2(m_pCodecCtx, m_pCodec, nullptr)<0) {
        qDebug() << "ffmpeg:  cannot open" << getLocalFileName();
        return ERR;
    }
    m_pResample = new EncoderFfmpegResample(m_pCodecCtx);
    m_pResample->open(m_pCodecCtx->channel_layout,m_pCodecCtx->sample_fmt,m_pCodecCtx->sample_rate,
                      AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_FLT,m_pCodecCtx->sample_rate);

    setChannelCount(2);
    setFrameRate(m_pCodecCtx->sample_rate);
    setFrameCount((m_pFormatCtx->duration * m_pCodecCtx->sample_rate) / AV_TIME_BASE);

    qDebug() << "ffmpeg: Samplerate: " << getFrameRate() << ", Channels: " << getChannelCount() << "\n";
    return OK;
}
void SoundSourceFFmpeg::close() {
    clearCache();
    if (m_pCodecCtx != nullptr) {
        qDebug() << "~SoundSourceFFmpeg(): Clear FFMPEG stuff";
        avcodec_close(m_pCodecCtx);
        avformat_close_input(&m_pFormatCtx);
    }
    if (m_pResample != nullptr) {
        qDebug() << "~SoundSourceFFmpeg(): Delete FFMPEG Resampler";
        delete m_pResample;
        m_pResample = nullptr;
    }
    while (m_SJumpPoints.size() > 0) {
        ffmpegLocationObject* l_SRmJmp = m_SJumpPoints[0];
        m_SJumpPoints.remove(0);
        free(l_SRmJmp);
    }
}
void SoundSourceFFmpeg::clearCache() {
    for(auto &&frame : m_SCache) av_frame_free(&frame);
    m_SCache.clear();
}
bool SoundSourceFFmpeg::readFramesToCache(SINT offset) {
    if(offset >= m_lCacheStartFrame && offset < m_lCacheEndFrame)return true;
    clearCache();
    if(offset >=0 && offset < m_lCacheStartFrame || offset > m_lCacheEndFrame){
      qint64 pts = (offset * AV_TIME_BASE)*1.0/getFrameRate();

      if(av_seek_frame(m_pFormatCtx, -1, pts,0)<0) return false;
      avformat_flush(m_pFormatCtx);
    }
    AVPacket l_pkt, l_free_pkt;
    AVFrame *l_frame = av_frame_alloc();
    if(!l_frame)return false;
    av_init_packet(&l_pkt);
    l_pkt.data = nullptr;
    l_pkt.size = 0;
    if(av_read_frame(m_pFormatCtx, &l_pkt)<0)
      return false;
    l_free_pkt = l_pkt;
    int64_t pts = AV_NOPTS_VALUE;
    AVRational time_base = {1,getFrameRate()};
    if(l_pkt.pts != AV_NOPTS_VALUE)
      pts = av_rescale_q(l_pkt.pts, time_base,m_pFormatCtx->streams[m_iAudioStream]->time_base);
    else if(l_pkt.dts != AV_NOPTS_VALUE)
      pts = av_rescale_q(l_pkt.pts, time_base,m_pFormatCtx->streams[m_iAudioStream]->time_base);
    m_lCacheStartFrame = pts;
    m_lCacheEndFrame = pts;
    while(l_pkt.size>0){
      int got_frame;
      int len = av_decode_audio4(m_pCodecCtx,l_frame,&got_frame,&l_pkt);
      if(len < 0) break;
      if(len==0)len=l_pkt.size;
      l_pkt.data += len;
      l_pkt.size -= len;
      if(!got_frame) continue;
      l_SCache.push_back(av_frame_alloc());
      AVFrame *s_frame = l_SCache.back();
      m_pResample->reSample(l_frame,s_frame);
      if(l_frame->pts!=AV_NOPTS_VALUE){
        s_frame->pts = av_rescale_q(l_frame->pts,time_base,m_pFormatCtx->streams[m_iAudioStream]->time_base);
      }else{
        s_frame->pts = m_lCacheEndFrame;
      }
      m_lCacheEndFrame = s_frame->pts+s_frame->nb_samples;
    }
    return true;
}
SINT SoundSourceFFmpeg::seekSampleFrame(SINT frameIndex) {
    readFramesToCache(frameIndex);
    if(frameIndex<= m_lCacheStartFrame){
      frameIndex  = m_lCacheStartFrame;
    }else if(frameIndex >= m_lCacheEndFrame){
      frameIndex  = m_lCacheEndFrame;
    }
    m_currentMixxxFrameIndex = frameIndex;
    return frameIndex;
}

SINT SoundSourceFFmpeg::readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer) {
    SINT originalNumber = numberOfFrames;
    while(numberOfFrames>0){
      AVFrame *s_frame = m_SCache.front();
      while(s_frame && s_frame->pts+s_frame->nb_samples <= m_currentMixxxFrameIndex){
        av_frame_free(&l_SCache[0]);
        l_SCache.pop_front();
        if(!l_SCache.size()){
          readFramesToCache(m_currentMixxxFrameIndex);
        }
        if(!l_SCache.size())break;
        s_frame = m_SCache.front();
      }
      SINT this_frame = math_min(s_frame->nb_samples - (m_currentMixxxFrameIndex-s_frame->pts),numberOfSamples);
      memmove(sampleBuffer,s_frame->extended_data[0],getChannelCount()*sizeof(CSAMPLE)*this_frame);
      sampleBuffer   += this_frame * getChannelCount();
      numberOfFrames -= this_frame;
      m_currentMixxxFrameIndex += this_frame;
    }
    return originalNumber - numberOfSamples;
}

} // namespace Mixxx
