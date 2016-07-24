
#include "sources/soundsourceffmpeg.h"
#include "util/performancetimer.h"
#include "util/timer.h"
#include "util/trace.h"
#include "util/math.h"
extern "C"{
  #include <libswresample/swresample.h>
}

namespace mixxx {

QStringList SoundSourceProviderFFmpeg::getSupportedFileExtensions() const
{
    av_register_all();
    avformat_network_init();
    avdevice_register_all();
    QStringList list;
    AVInputFormat *l_SInputFmt  = nullptr;
    while ((l_SInputFmt = av_iformat_next(l_SInputFmt))) {
        if (!l_SInputFmt->name)
            continue;
        auto names = QString(l_SInputFmt->name).split(",");
        qDebug() << "Found names " << names;
        for( const auto &name : names )
            if(!list.contains(name))
                list.append(name);
    }
    return list;
}
SoundSourceFFmpeg::SoundSourceFFmpeg(QUrl url)
    : SoundSource(url)

{
    av_register_all();
    avformat_network_init();
}
SoundSourceFFmpeg::~SoundSourceFFmpeg()
{
    close();
}
int64_t SoundSourceFFmpeg::getChannelLayout() const
{
    return av_get_default_channel_layout(getChannelCount());
}
SoundSource::OpenResult SoundSourceFFmpeg::tryOpen(const AudioSourceConfig& audioSrcCfg)
{
    auto qBAFilename = getLocalFileName().toLocal8Bit();
    m_fmt_ctx = avformat_alloc_context();
    m_swr = swr_alloc();
    if(!m_packet || !m_frame_dec || !m_frame_swr || !m_swr || !m_fmt_ctx)
        return OpenResult::FAILED;
    // Open file and make m_pFormatCtx
    if (avformat_open_input(&m_fmt_ctx, qBAFilename.constData(), nullptr,nullptr)) {
        qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open" << qBAFilename;
        return OpenResult::FAILED;
    }
    // Retrieve stream information
    if (avformat_find_stream_info(m_fmt_ctx, nullptr) < 0) {
        qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open" << qBAFilename;
        return OpenResult::FAILED;
    }
    std::for_each(&m_fmt_ctx->streams[0],&m_fmt_ctx->streams[m_fmt_ctx->nb_streams],[](auto *stream){
            stream->discard = AVDISCARD_ALL;
        });
    std::for_each(&m_fmt_ctx->streams[0],&m_fmt_ctx->streams[m_fmt_ctx->nb_streams],[&, this](auto *stream){
            if(stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                qDebug() << "Found an attached picture stream: " << stream->index;
                av_dump_format(m_fmt_ctx, stream->index, qBAFilename.constData(), false);
                stream->discard = AVDISCARD_NONE;
                auto ctx = avcodec_alloc_context3(avcodec_find_decoder(stream->codecpar->codec_id));
                avcodec_parameters_to_context(ctx, stream->codecpar);
                avcodec_open2(ctx,nullptr,nullptr);
                char buf[4096];
                avcodec_string(buf, sizeof(buf), ctx, false);
                avcodec_close(ctx);
                avcodec_free_context(&ctx);
                qDebug() << "Attached pictures have associated codec\n" 
                    << buf;
            }
        });
    auto stream_idx = -1;
    //debug only (Enable if needed)
    av_dump_format(m_fmt_ctx, 0, qBAFilename.constData(), false);
    if((stream_idx=av_find_best_stream(m_fmt_ctx,AVMEDIA_TYPE_AUDIO,-1,-1,&m_codec,0))<0){
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot find audio stream in" << qBAFilename;
      return OpenResult::FAILED;
    }
    m_stream = m_fmt_ctx->streams[stream_idx];
    m_stream->discard = AVDISCARD_NONE;
    m_stream_tb = m_stream->time_base;
    if(!m_codec && !(m_codec = avcodec_find_decoder(m_stream->codecpar->codec_id))){
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot find decoder for audio stream in" << qBAFilename;
      return OpenResult::FAILED;
    }
    m_codec_ctx    = avcodec_alloc_context3 ( m_codec );
    if(!m_codec_ctx) {
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot alloc codec context for" << qBAFilename;
      return OpenResult::FAILED;
    }
    avcodec_parameters_to_context( m_codec_ctx, m_stream->codecpar );
    if(avcodec_open2(m_codec_ctx, m_codec,nullptr)<0){
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open codec for" << qBAFilename;
      return OpenResult::FAILED;
    }
//    setChannelCount(m_codec_ctx->channels);
    setChannelCount(2);
    setSamplingRate(m_codec_ctx->sample_rate);
    m_output_tb = AVRational{1,(int)getSamplingRate()};
    setFrameCount(av_rescale_q(m_fmt_ctx->duration,AV_TIME_BASE_Q,m_output_tb));
    qDebug() << "SoundSourceFFmpeg::tryOpen: Samplerate: " << getSamplingRate() << ", Channels: " << getChannelCount() << "\n";
    m_swr = swr_alloc_set_opts(m_swr, 
                               getChannelLayout(),
                               AV_SAMPLE_FMT_FLT,
                               getSamplingRate(),
                               m_codec_ctx->channel_layout,
                               m_codec_ctx->sample_fmt,
                               m_codec_ctx->sample_rate,
                               0,
                               nullptr
      );
    if(swr_init(m_swr)<0){
      qDebug() << "SoundSourceFFmpeg::tryOpen: failed to initialize resampler for " << qBAFilename;
      return OpenResult::FAILED;
    }
    m_frame_swr.unref();
    m_frame_swr->format         = AV_SAMPLE_FMT_FLT;
    m_frame_swr->channels       = getChannelCount();
    m_frame_swr->channel_layout = getChannelLayout();
    m_frame_swr->sample_rate    = getSamplingRate();
    m_frame_swr->nb_samples     = 0;
    next();
    m_sample_now = m_sample_frame;
    m_sample_origin = 0;
    m_sample_origin = seekSampleFrame(0);
    return OpenResult::SUCCEEDED;
}
void SoundSourceFFmpeg::close()
{
    avformat_close_input(&m_fmt_ctx);
    if(m_codec_ctx) {
        if(avcodec_is_open(m_codec_ctx))
            avcodec_close(m_codec_ctx);
        avcodec_free_context(&m_codec_ctx);
    }
    swr_free(&m_swr);
}
bool SoundSourceFFmpeg::next()
{
    auto ret = 0;
    av_frame_unref(m_frame_dec);
    while((ret = avcodec_receive_frame(m_codec_ctx, m_frame_dec)) < 0) {
        if(ret != AVERROR(EAGAIN))
            return false;
        av_packet_unref(m_packet);
        if((ret = av_read_frame(m_fmt_ctx, m_packet)) < 0) {
            if(ret == AVERROR_EOF) {
                if((ret = avcodec_send_packet(m_codec_ctx, nullptr)) < 0)
                    return false;
                continue;
            }else{
                return false;
            }
        }
        if(m_packet->stream_index != m_stream->index) {
            m_packet.unref();
            continue;
        }
        if((ret = avcodec_send_packet(m_codec_ctx, m_packet )) < 0) {
            if(ret == AVERROR_EOF) {
                avcodec_flush_buffers(m_codec_ctx);
                if((ret = avcodec_send_packet(m_codec_ctx, m_packet))>=0)
                    continue;
            }
            if(ret != AVERROR(EAGAIN))
                return false;
        }
    }
    m_frame_dec->pts = av_frame_get_best_effort_timestamp(m_frame_dec);
    m_sample_frame += m_frame_swr->nb_samples;
    m_frame_swr.unref();
    m_frame_swr->nb_samples = 0;
    m_frame_swr->format = AV_SAMPLE_FMT_FLT;
    m_frame_swr->channel_layout = getChannelLayout();
    m_frame_swr->channels = getChannelCount();
    m_frame_swr->sample_rate = getSamplingRate();
    auto delay = swr_get_delay(m_swr, getSamplingRate());
    m_frame_swr->pts = m_frame_dec->pts - av_rescale_q(delay, m_output_tb, m_stream_tb);
    if ( swr_convert_frame ( m_swr, m_frame_swr, m_frame_dec) < 0 ){
      if ( swr_config_frame ( m_swr, m_frame_swr, m_frame_dec ) < 0 
      ||   swr_convert_frame ( m_swr, m_frame_swr, m_frame_dec ) < 0 ){
        return false;
      }
    }
    m_sample_frame = av_rescale_q(m_frame_swr->pts, m_stream_tb, m_output_tb);
    return true;
}
int64_t SoundSourceFFmpeg::seekSampleFrame(int64_t frameIndex)
{
    ScopedTimer _t("SoundSourceFFmpeg::seekSampleFrame");
    frameIndex += m_sample_origin;
    if ( frameIndex >= m_sample_frame && frameIndex < m_sample_frame + m_frame_swr->nb_samples * 4 ){
      while ( frameIndex > m_sample_frame + m_frame_swr->nb_samples && next())
      {}
    }
    if ( frameIndex >= m_sample_frame && frameIndex < m_sample_now + m_frame_swr->nb_samples){
      m_sample_now = frameIndex;
    }else{
      auto delay = 0;
      if(m_swr && swr_is_initialized(m_swr)) {
        delay = swr_get_delay(m_swr, getSamplingRate());
      }
      auto framePts = av_rescale_q ( frameIndex - delay, m_output_tb,AV_TIME_BASE_Q);
      avformat_seek_file( m_fmt_ctx, -1, INT64_MIN,framePts, framePts, 0 );
      avcodec_flush_buffers(m_codec_ctx);
      av_packet_unref(m_packet);
      if(next()){
        if ( m_sample_frame + delay > frameIndex )
            m_sample_now = m_sample_frame + delay;
        else 
            m_sample_now = frameIndex;
      }else{
        m_sample_now = m_sample_frame;
      }
    }
    return m_sample_now;
}
int64_t SoundSourceFFmpeg::readSampleFrames(int64_t numberOfFrames,CSAMPLE* sampleBuffer)
{
    ScopedTimer _t("SoundSourceFFmpeg::readSampleFrames");
    if(!numberOfFrames)
        return 0;
    auto numberLeft = numberOfFrames;
    while(m_sample_now - m_sample_frame > m_frame_swr->nb_samples){
        if(!next())
            return 0;
    }
    while(numberLeft) {
      auto offset_here = m_sample_now - m_sample_frame;
      auto count_here  = std::min(m_frame_swr->nb_samples - offset_here, numberLeft);
      if ( count_here > 0 ) {
          uint8_t *tmp = reinterpret_cast<uint8_t*>(sampleBuffer);
          av_samples_copy(&tmp, m_frame_swr->extended_data, 0, offset_here, count_here, getChannelCount(), static_cast<AVSampleFormat>(m_frame_swr->format));
        sampleBuffer += getChannelCount() * count_here;
        m_sample_now += count_here;
        numberLeft   -= count_here;
      }else if (!next()){break;}
    }
    return numberOfFrames - numberLeft;
}
} // namespace mixxx
