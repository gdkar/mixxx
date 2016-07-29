
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
    if(!m_packet || !m_frame_dec || !m_frame_swr || !m_swr || !m_fmt_ctx)
        return OpenResult::FAILED;
    // Open file and make m_pFormatCtx
    if (m_fmt_ctx.open_input( qBAFilename.constData()) < 0) {
        qDebug() << "SoundSourceFFmpeg::tryOpen: cannot open" << qBAFilename;
        return OpenResult::FAILED;
    }
    std::for_each(&m_fmt_ctx->streams[0],&m_fmt_ctx->streams[m_fmt_ctx->nb_streams],[&, this](auto *stream){
            if(stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                qDebug() << "Found an attached picture stream: " << stream->index;
                av_dump_format(m_fmt_ctx, stream->index, qBAFilename.constData(), false);
                stream->discard = AVDISCARD_NONE;
                auto ctx = codec_context{stream->codecpar};
                ctx.open();
                char buf[4096];
                avcodec_string(buf, sizeof(buf), ctx, false);
                qDebug() << "Attached pictures have associated codec\n"  << buf;
            }
        });
    //debug only (Enable if needed)
    std::tie(m_stream,m_codec) = m_fmt_ctx.find_best_stream(AVMEDIA_TYPE_AUDIO);
    m_stream_tb = m_stream->time_base;
    if(!m_codec){
      qDebug() << "SoundSourceFFmpeg::tryOpen: cannot find decoder for audio stream in" << qBAFilename;
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
    m_swr.set_opts(
        getChannelLayout(),
        AV_SAMPLE_FMT_FLT,
        getSamplingRate(),
        m_codec_ctx->channel_layout,
        m_codec_ctx->sample_fmt,
        m_codec_ctx->sample_rate,
        0,
        nullptr
      );
    if(m_swr.init()<0){
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
    m_fmt_ctx.close();
    m_codec_ctx.close();
}
bool SoundSourceFFmpeg::next()
{
    auto ret = 0;
    m_frame_dec.unref();
    while((ret = m_codec_ctx.receive_frame( m_frame_dec)) < 0) {
        if(ret != AVERROR(EAGAIN))
            return false;
        m_packet.unref();
        if((ret = m_fmt_ctx.read_frame( m_packet)) < 0) {
            if(ret == AVERROR_EOF) {
                if((ret = m_codec_ctx.send_packet( nullptr)) < 0)
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
        if((ret = m_codec_ctx.send_packet( m_packet )) < 0) {
            if(ret == AVERROR_EOF) {
                m_codec_ctx.flush_buffers();
                if((ret = m_codec_ctx.send_packet( m_packet))>=0)
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
    auto delay = m_swr.delay(getSamplingRate());
    m_frame_swr->pts = m_frame_dec->pts - av_rescale_q(delay, m_output_tb, m_stream_tb);
    if (  m_swr.convert( m_frame_swr, m_frame_dec) < 0 ){
      if ( m_swr.config( m_frame_swr, m_frame_dec ) < 0 
      ||   m_swr.convert( m_frame_swr, m_frame_dec ) < 0 ){
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
      if(m_swr && m_swr.initialized()) {
        delay = m_swr.delay( getSamplingRate());
      }
      auto framePts = av_rescale_q ( frameIndex - delay, m_output_tb,AV_TIME_BASE_Q);
      m_fmt_ctx.seek_file( -1, INT64_MIN,framePts, framePts);
      m_codec_ctx.flush_buffers();
      m_packet.unref();
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
