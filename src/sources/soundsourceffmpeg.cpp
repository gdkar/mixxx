
#include "sources/soundsourceffmpeg.h"
extern "C"{
  #include <libswresample/swresample.h>
}
#define AUDIOSOURCEFFMPEG_CACHESIZE 1000
#define AUDIOSOURCEFFMPEG_MIXXXFRAME_TO_BYTEOFFSET (sizeof(CSAMPLE) * getChannelCount())
#define AUDIOSOURCEFFMPEG_FILL_FROM_CURRENTPOS -1

namespace mixxx {

QStringList SoundSourceProviderFFmpeg::getSupportedFileExtensions() const {
    QStringList list;
    AVInputFormat *l_SInputFmt  = NULL;

    while ((l_SInputFmt = av_iformat_next(l_SInputFmt))) {
        if (l_SInputFmt->name == NULL) {break;}
        if (!strcmp(l_SInputFmt->name, "flac")) {
            list.append("flac");
        } else if (!strcmp(l_SInputFmt->name, "ogg")) {
            list.append("ogg");
        } else if (!strcmp(l_SInputFmt->name, "mov,mp4,m4a,3gp,3g2,mj2")) {
            list.append("m4a");
            list.append("mp4");
        } else if (!strcmp(l_SInputFmt->name, "mp4")) {
            list.append("mp4");
        } else if (!strcmp(l_SInputFmt->name, "mp3")) {
            list.append("mp3");
        } else if (!strcmp(l_SInputFmt->name, "aac")) {
            list.append("aac");
        } else if (!strcmp(l_SInputFmt->name, "opus") ||
                   !strcmp(l_SInputFmt->name, "libopus")) {
            list.append("opus");
        } else if (!strcmp(l_SInputFmt->name, "wma") or
                   !strcmp(l_SInputFmt->name, "xwma")) {
            list.append("wma");
        }else{
          auto names = QString(l_SInputFmt->name).split(",");
          for( auto &name : names ){
            if(!list.contains(name)) list.append(name);
          }
        }
    }

    return list;
}

SoundSourceFFmpeg::SoundSourceFFmpeg(QUrl url)
    : SoundSource(url)
{}

SoundSourceFFmpeg::~SoundSourceFFmpeg() { close(); }
Result SoundSourceFFmpeg::tryOpen(const AudioSourceConfig& /*audioSrcCfg*/) {
    auto l_format_opts = (AVDictionary *)nullptr;
    const auto qBAFilename = getLocalFileNameBytes();
    qDebug() << "New SoundSourceFFmpeg :" << qBAFilename;
    // Open file and make m_pFormatCtx
    m_fmt_ctx = avformat_alloc_context();
    if (avformat_open_input(&m_fmt_ctx, qBAFilename.constData(), nullptr,nullptr)) {
        qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open" << qBAFilename;
        return ERR;
    }
    // Retrieve stream information
    if (avformat_find_stream_info(m_fmt_ctx, nullptr) < 0) {
        qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open" << qBAFilename;
        return ERR;
    }
    //debug only (Enable if needed)
    //av_dump_format(m_pFormatCtx, 0, qBAFilename.constData(), false);
    if((m_stream_idx=av_find_best_stream(m_fmt_ctx,AVMEDIA_TYPE_AUDIO,-1,-1,&m_dec,0))<0){
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot find audio stream in" << qBAFilename;
      return ERR;
    }
    auto m_stream    = m_fmt_ctx->streams[m_stream_idx];
    if(!m_dec && !(m_dec = avcodec_find_decoder(m_stream->codec->codec_id))){
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot find decoder for audio stream in" << qBAFilename;
    }

    m_dec_ctx    = avcodec_alloc_context3 ( m_dec );
    av_opt_set_int(m_dec_ctx,"refcounted_frames",1,0);
    avcodec_copy_context ( m_dec_ctx, stream->codec );
    if(avcodec_open2(m_dec_ctx, m_dec,nullptr)<0){
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open codec for" << qBAFilename;
    }

    m_stream_tb = m_stream->time_base;
    setChannelCount(m_dec_ctx->channels);
    setFrameRate(m_dec_ctx->sample_rate);
    setFrameCount(static_cast<qint64>(static_cast<double>(m_fmt_ctx->duration) * getFrameRate() / AV_TIME_BASE));
    qDebug() << "SoundSourceFFmpeg::tryOpen: Samplerate: " << getFrameRate() << ", Channels: " << getChannelCount() << "\n";
    m_swr = swr_alloc_set_opts(m_swr, 
                               AV_CH_LAYOUT_STEREO,
                               AV_SAMPLE_FMT_FLT,
                               getFrameRate(),
                               m_codec_ctx->channel_layout,
                               m_codec_ctx->sample_fmt,
                               m_codec_ctx->sample_rate,
                               0,
                               nullptr
      );
    if(swr_init(m_swr)<0){
      qDebug() << "SoundSourceFFmpeg::tryOpen: failed to initialize resampler for " << qBAFilename;
      return ERR;
    }
    if(!(m_frame = av_frame_alloc ())){
      qDebug() << "SoundSourceFFmpeg::tryOpen: failed to allocate frame when opening " << qBAFilename;
    }
    m_frame->format         = AV_SAMPLE_FMT_FLT;
    m_frame->channel_layout = AV_CH_LAYOUT_STEREO;
    m_frame->nb_samples     = 0;

    m_packet.data = nullptr;
    m_packet.size = 0;
    av_init_packet(&m_packet);
    m_packet_free = m_packet;
    return OK;
}
void SoundSourceFFmpeg::close() {
    avformat_close_input( &m_fmt_ctx );
    avcodec_close(m_dec_ctx);
    av_freep(&m_dec_ctx);
    swr_free(&m_swr);
    if ( m_packet_free.data || m_packet_free.size ) av_free_packet ( &m_packet_free );
    av_frame_free (&m_frame);
}
bool SoundSourceFFmpeg::getNextFrame(){
  auto ret = 0;
  while ((! m_packet.size || !m_packet.data) && ret == 0){
    ret = av_read_frame ( m_fmt_ctx, &m_packet_free );
    if ( ret < 0 || m_packet_free->stream_index != m_stream_idx){
      av_free_packet ( & m_packet_free );
    }else{
      m_packet      = m_packet_free;
      if ( m_packet.pts != AV_NOPTS_VALUE ){
        m_offset_base = av_rescale_q ( m_packet.pts, AV_TIME_BASE_Q, AVRational { 1, getFrameRate()});
      }
    }
  }
  if ( ret < 0 ) return false;
  auto got_frame = 0;
  auto dec_frame = av_frame_alloc ();
  ret = av_decode_audio4 ( m_dec_ctx, dec_frame, &got_frame, &m_packet );
  if ( ret <= 0 ){ret = m_packet.size;}
  m_packet.size -= ret;
  m_packet.data += ret;
  if ( m_packet.size <= 0 ){
    av_free_packet ( &m_packet_free );
    m_packet.size = 0;
    m_packet.data = nullptr;
  }
  if ( got_frame ){
    dec_frame->pts = av_get_best_effort_timestamp ( dec_frame );
    m_offset_base += m_frame->nb_samples;
    av_frame_unref ( &m_frame );
    m_frame->nb_samples = 0;
    m_frame->format = AV_SAMPLE_FMT_FLT;
    m_frame->channel_layout = AV_CH_LAYOUT_STEREO;
    if ( swr_convert_frame ( m_swr, m_frame, dec_frame ) < 0 ){
      if ( swr_config_frame ( m_swr, m_frame, dec_frame ) < 0 
      ||   swr_convert_frame ( m_swr, m_frame, dec_frame ) < 0 ){
        return false;
      }
    }
    if ( dec_frame->pts != AV_NOPTS_VALUE ){
      m_offset_base = av_rescale_q ( dec_frame->pts, AV_TIME_BASE_Q,AVRational{1,getFrameRate()});
    }
    av_frame_free ( & dec_frame );
    return true;
  }
  return false;
}
SINT SoundSourceFFmpeg::seekSampleFrame(SINT frameIndex) {
    DEBUG_ASSERT(isValidFrameIndex(frameIndex));
    if ( frameIndex >= m_offset_base && frameIndex < m_offset_base + m_frame->nb_samples * 4 ){
      while ( frameIndex > m_offset_base + m_frame->nb_samples && getNextFrame())
      {}
    }
    if ( frameIndex >= m_offset_base && frameIndex < m_offset_base + m_frame->nb_samples){
      m_offset_cur = frameIndex;
    }else{
      auto framePts = av_rescale_q ( frameIndex, AVRational{1,(int)getFrameRate()},AV_TIME_BASE_Q);
      av_seek_frame ( m_fmt_ctx, -1, framePts, 0 );
      m_packet.size = 0;
      m_packet.data = nullptr;
      if(getNextFrame()){
        if ( m_offset_base > frameIndex ) m_offset_cur = m_offset_base;
        else if ( m_offset_base + m_frame->nb_samples <= frameIndex ) m_offset_cur = m_offset_base + m_frame->nb_samples;
        else m_offset_cur = frameIndex;
      }else{
        m_offet_cur = m_offset_base;
      }
    }
    return m_offset_cur;
}
SINT SoundSourceFFmpeg::readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer) {
    if(!numberOfFrames) return 0;
    auto numberLeft = numberOfFrames;
    while(numberLeft){
      auto offset_here = m_offset_cur - m_offset_base;
      auto count_here  = std::min(m_frame->nb_samples - offset_here, numberLeft);
      if ( count_here > 0 ){
        std::memmove ( sampleBuffer, m_frame->extended_data[0] + ( getChannelCount() * sizeof(CSAMPLE) * offset_here ),
            getChannelCount() * sizeof(CSAMPLE) * count_here );
        sampleBuffer += getChannelCount() * count_here;
        m_offset_cur += count_here;
        numberLeft   -= count_here;
      }else if (!getNextFrame()){break;}
    }
    return numberOfFrames - numberLeft;
}

} // namespace mixxx
