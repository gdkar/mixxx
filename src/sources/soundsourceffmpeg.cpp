#include "sources/soundsourceffmpeg.h"

namespace mixxx {
QStringList SoundSourceProviderFFmpeg::getSupportedFileExtensions() const {
    av_register_all();
    avformat_network_init();
    auto list = QStringList{};
    auto l_SInputFmt = static_cast<AVInputFormat *>(0);
    while ((l_SInputFmt = av_iformat_next(l_SInputFmt)) && l_SInputFmt->name) {
        if (!strcmp(l_SInputFmt->name, "flac")) {list.append("flac");}
        else if (!strcmp(l_SInputFmt->name, "ogg")) {list.append("ogg");}
        else if (!strcmp(l_SInputFmt->name, "mov,mp4,m4a,3gp,3g2,mj2")) {
            list.append("m4a");
            list.append("mp4");
        } else if (!strcmp(l_SInputFmt->name, "mp4")) {list.append("mp4");}
        else if (!strcmp(l_SInputFmt->name, "mp3")) {list.append("mp3");}
        else if (!strcmp(l_SInputFmt->name, "aac")) {list.append("aac");}
        else if (!strcmp(l_SInputFmt->name, "opus") || !strcmp(l_SInputFmt->name, "libopus")) {list.append("opus");}
        else if (!strcmp(l_SInputFmt->name, "wma")  ||  !strcmp(l_SInputFmt->name, "xwma")) {list.append("wma");}
        auto names = QString(l_SInputFmt->name).split(".");
        for ( auto &name : names ) {
          if ( ! list.contains ( name ) ) list.append ( name );
        }
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
SoundSource::OpenResult SoundSourceFFmpeg::tryOpen(const AudioSourceConfig &config)
{
    const auto filename = getLocalFileName().toLocal8Bit();
    auto ret = 0;
    qDebug() << __FUNCTION__  << "(" << filename << ")";
    // Open file and make m_pFormatCtx
    if (( ret = m_format_ctx.open_input(filename.constData()))<0) {
        qDebug() << __FUNCTION__ << "cannot open" << filename << av_strerror ( ret );
        return OpenResult::FAILED;
    }

    //debug only (Enable if needed)
    for ( auto i = 0u; i < m_format_ctx->nb_streams; i++) {
        if(m_format_ctx->streams[i]->disposition &AV_DISPOSITION_ATTACHED_PIC)
            m_format_ctx->streams[i]->discard = AVDISCARD_NONE;
        else
            m_format_ctx->streams[i]->discard = AVDISCARD_ALL;
    }
    std::tie(m_stream, m_codec) = m_format_ctx.find_best_stream(AVMEDIA_TYPE_AUDIO);
    if(!m_codec || !m_stream)
        return OpenResult::FAILED;

    m_format_ctx.dump( m_stream->index, filename.constData());
    m_stream->discard = AVDISCARD_NONE;
    m_codec_ctx = codec_context(m_stream->codecpar);
    if(m_codec_ctx.open() < 0)
        return OpenResult::FAILED;

    m_stream_tb = m_stream->time_base;
    m_codec_tb  = m_codec_ctx->time_base;
    if ( !m_codec_ctx->channel_layout ) {
      m_codec_ctx->channel_layout = av_get_default_channel_layout(m_codec_ctx->channels);
    } else if ( !m_codec_ctx->channels ) {
      m_codec_ctx->channels = av_get_channel_layout_nb_channels ( m_codec_ctx->channel_layout );
    } if ( !m_codec_ctx->channel_layout || !m_codec_ctx->channels ) {
      m_codec_ctx->channels = 2;
      m_codec_ctx->channel_layout = av_get_default_channel_layout(m_codec_ctx->channels);
    }
    if ( config.getChannelCount() <= 0 ) setChannelCount( m_codec_ctx->channels );
    else                                setChannelCount( config.getChannelCount());
    if ( config.getSamplingRate() < 8000 )  setSamplingRate( m_codec_ctx->sample_rate );
    else                                setSamplingRate( config.getSamplingRate());

    m_output_tb = AVRational{1,static_cast<int>(getSamplingRate())};
    setFrameCount ( av_rescale_q( m_format_ctx->duration, m_stream_tb, m_output_tb ) );
    m_frame->channel_layout = av_get_default_channel_layout ( getChannelCount() );
    m_frame->format         = AV_SAMPLE_FMT_FLT;
    m_frame->sample_rate    = getSamplingRate();
    auto discarded = size_t{0};
    auto total_size= size_t{0};
    do {
      ret = m_format_ctx.read_frame(m_packet );
      if ( ret < 0 || m_packet->stream_index != m_stream->index) {
        if ( m_packet->stream_index != m_stream->index )
            discarded ++;
        else if ( ret != AVERROR_EOF )
          qDebug() << __FUNCTION__ << filename << "av_read_packet returned error" << av_strerror(ret);
        m_packet.unref();
      } else {
        m_pkt_array.emplace_back(m_packet);
        total_size += m_packet->size;
        m_packet.unref();
      }
    }while ( ret >= 0 );
    if ( discarded )
        qDebug() << __FUNCTION__ << "discarded" << discarded << "packets from other streams when demuxing" << filename;
    if ( m_stream->start_time != AV_NOPTS_VALUE )
        m_first_pts = m_stream->start_time;
    else if ( m_pkt_array.size() )
        m_first_pts = m_pkt_array.front()->pts;
    m_pkt_index = 0;
    if ( m_pkt_index < decltype(m_pkt_index)(m_pkt_array.size() ) )
        m_packet = m_pkt_array.at(m_pkt_index);
    setFrameCount ( av_rescale_q ( m_pkt_array.back()->pts + m_pkt_array.back()->duration - m_first_pts, m_stream_tb, m_output_tb ) );

    qDebug() << __FUNCTION__ << QString{": demuxing results for %1"}.arg(getLocalFileName()) << "\n"
                             << QString{"  frameRate = %1"}.arg(getSamplingRate()) << "\n"
                             << QString{"  channelCount = %L1"}.arg(getChannelCount()) << "\n"
                             << QString{"  frameCount = %L1"}.arg(getFrameCount()) << "\n"
                             << QString{"  total packets = %L1"}.arg(m_pkt_array.size()) << "\n"
                             << QString{"  total demuxed size = %L1 bytes"}.arg(total_size) << "\n\n" ;
    {
        auto tag = static_cast<AVDictionaryEntry*>(nullptr);
        auto tags = m_format_ctx->metadata;
        while((tag = av_dict_get(tags,"",tag,AV_DICT_IGNORE_SUFFIX))) {
            qDebug() << QString{"[%1] = %2"}.arg(QString{tag->key},QString{tag->value});
        }
    }
    decode_next_frame ();
    return OpenResult::SUCCEEDED;
}
void SoundSourceFFmpeg::close() {
    m_swr.close();
    m_codec_ctx.close();
    m_format_ctx.close();
    m_orig_frame.unref();
    m_frame.unref();
    m_pkt_array.clear();

    m_stream       = nullptr;
    m_stream_index = -1;
    m_codec        = nullptr;
    m_first_pts    = 0;
}
SINT SoundSourceFFmpeg::seekSampleFrame(SINT frameIndex)
{
    DEBUG_ASSERT(isValidFrameIndex(frameIndex));
    if ( m_frame->pts == AV_NOPTS_VALUE && !decode_next_frame ( ) )
        return -1;
    auto first_sample = av_rescale_q ( m_frame->pts-m_first_pts, m_stream_tb, m_output_tb );
    auto frame_pts    = av_rescale_q ( frameIndex, m_output_tb,m_stream_tb);
    if ( frameIndex >= first_sample && frameIndex  < first_sample + m_frame->nb_samples ) {
      m_offset = frameIndex - first_sample;
      return     frameIndex;
    }
    if ( frame_pts < ( m_pkt_array.front()->pts - m_first_pts ) ){
      m_pkt_index =  0;
      m_codec_ctx.flush_buffers();
      decode_next_frame ( );
      first_sample = av_rescale_q ( m_frame->pts - m_first_pts, m_stream_tb, m_output_tb );
      m_offset = 0;
      return first_sample;
    }
    if ( frame_pts >= ( m_pkt_array.back()->pts - m_first_pts ) ) {
      m_pkt_index  = m_pkt_array.size() - 2;
      m_codec_ctx.flush_buffers();
      decode_next_frame ( );
      first_sample = av_rescale_q ( m_frame->pts - m_first_pts, m_stream_tb, m_output_tb );
      m_offset     = frameIndex - first_sample;
      return frameIndex;
    }
    auto hindex = m_pkt_array.size() - 1;
    auto lindex = decltype(hindex){0};
    auto lpts   = m_pkt_array.at(lindex)->pts - m_first_pts;
    auto bail   = false;
    m_pkt_index = -1;
    while ( hindex > lindex + 1) {
      auto pts_dist   = m_pkt_array.at(hindex)->pts - m_pkt_array.at(lindex)->pts;
      auto idx_dist   = hindex - lindex;
      auto target_dist= frame_pts - ( lpts );
      auto mindex = lindex + (( target_dist * idx_dist ) / pts_dist);
      if ( mindex >= hindex ) mindex = hindex - 1;
      if ( mindex <= lindex ) {
        if ( bail ) mindex = ( idx_dist / 2 ) + lindex;
        else {
          mindex = lindex + 1;
          bail = true;
        }
      } else {
        bail = false;
      }
      auto mpts = m_pkt_array.at(mindex  )->pts - m_first_pts;
      auto npts = m_pkt_array.at(mindex+1)->pts - m_first_pts;
      if ( mpts > frame_pts ) {
        hindex = mindex;
      }else if ( npts <= frame_pts ) {
        lindex = mindex + 1;
        lpts   = npts;
      } else {
        lindex = mindex;
        lpts   = mpts;
        hindex = mindex + 1;
      }
    }
    m_pkt_index = lindex;
    if ( m_pkt_index > 0 )
        m_pkt_index--;
    if ( m_pkt_index > 0 )
        m_pkt_index--;
    avcodec_flush_buffers(m_codec_ctx);
    decode_next_frame ();
    first_sample = av_rescale_q ( m_frame->pts - m_first_pts, m_stream_tb, m_output_tb );
    m_offset = frameIndex - first_sample;
    return     frameIndex;
}
bool SoundSourceFFmpeg::decode_next_frame(){
    auto ret = 0;
    while ((m_orig_frame.unref(),(ret = m_codec_ctx.receive_frame(m_orig_frame))) < 0) {
        if(ret == AVERROR_EOF) {
            if(m_pkt_index> int64_t(m_pkt_array.size())) {
                return false;
            }else if(m_pkt_index == int64_t(m_pkt_array.size())) {
                m_codec_ctx.send_packet(nullptr);
                m_pkt_index++;
            } else {
                m_codec_ctx.flush_buffers();
            }
        }else if(ret != AVERROR(EAGAIN)) {
            return false;
        }
        if(m_pkt_index == int64_t(m_pkt_array.size())) {
            m_pkt_index++;
            if((ret = m_codec_ctx.send_packet(nullptr)) < 0)
                return false;
        }else if((ret = m_codec_ctx.send_packet(m_pkt_array.at(m_pkt_index++))) < 0)
            return false;
    }
    if ( !m_orig_frame->sample_rate    )
        m_orig_frame->sample_rate    = m_codec_ctx->sample_rate;
    if ( !m_orig_frame->channel_layout )
        m_orig_frame->channel_layout = m_codec_ctx->channel_layout;
    if ( !m_orig_frame->channels )
        m_orig_frame->channels       = m_codec_ctx->channels;

    m_orig_frame->pts = m_orig_frame.best_effort_timestamp();

    m_frame.unref();
    m_frame->format         = AV_SAMPLE_FMT_FLT;
    m_frame->channel_layout = av_get_default_channel_layout ( getChannelCount() );
    m_frame->sample_rate    = getSamplingRate();
    auto delay = 0;
    if(!m_swr.initialized() && (m_swr.config(m_frame,m_orig_frame) < 0
        || m_swr.init() < 0)) {
        qDebug() << __FUNCTION__ << "error initializing SwrContext" << av_strerror ( ret );
    }else{
        delay = swr_get_delay ( m_swr, getSamplingRate( ) );
    }
    m_frame->pts     = m_orig_frame->pts - av_rescale_q ( delay, m_output_tb, m_stream_tb ) ;
    m_frame->pkt_pts = m_orig_frame->pkt_pts;
    m_frame->pkt_dts = m_orig_frame->pkt_dts;
    if((ret = m_swr.convert(m_frame,m_orig_frame)) < 0) {
        qDebug() << __FUNCTION__ << "error converting frame for" << getLocalFileName()<< av_strerror(ret);
        return false;
    }
    return true;
}
SINT SoundSourceFFmpeg::readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer) {
    auto number_done   = decltype(numberOfFrames){0};
    uint8_t *dst[] = {reinterpret_cast<uint8_t*>(sampleBuffer)};
    while ( number_done < numberOfFrames) {
      if ( m_offset >= m_frame->nb_samples ) {
        m_offset -= m_frame->nb_samples;
        if ( !decode_next_frame() )
            break;
      } else {
        auto available_here = m_frame->nb_samples - m_offset;
        auto needed_here    = numberOfFrames - number_done;
        auto this_chunk     = std::min<SINT>(available_here, needed_here);
        if ( this_chunk <= 0 ) break;
        if ( sampleBuffer ) {
          av_samples_copy (
              dst,
              m_frame->data,
              number_done,
              m_offset,
              this_chunk,
              getChannelCount(),
              AV_SAMPLE_FMT_FLT
            );
        }
        number_done += this_chunk;
        m_offset    += this_chunk;
      }
    }
    return number_done;
}
} // namespace Mixxx
