
#include "sources/soundsourceffmpeg.h"
extern "C"{
#include <libswresample/swresample.h>
};

#define AUDIOSOURCEFFMPEG_CACHESIZE 1000
#define AUDIOSOURCEFFMPEG_MIXXXFRAME_TO_BYTEOFFSET (sizeof(CSAMPLE) * getChannelCount())
#define AUDIOSOURCEFFMPEG_FILL_FROM_CURRENTPOS -1

namespace Mixxx {

QStringList SoundSourceProviderFFmpeg::getSupportedFileExtensions() const {
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
        else {
          auto names = QString(l_SInputFmt->name).split(".");
          for ( auto &name : names ) {
            if ( ! list.contains ( name ) ) list.append ( name );
          }
        }
    }
    return list;
}

SoundSourceFFmpeg::SoundSourceFFmpeg(QUrl url)
    : SoundSource(url),
      m_format_ctx(avformat_alloc_context())
{}

SoundSourceFFmpeg::~SoundSourceFFmpeg() {
    close();
}

Result SoundSourceFFmpeg::tryOpen(const AudioSourceConfig& config) {
    av_register_all();
    auto l_format_opts = (AVDictionary *)nullptr;
    const auto filename = getLocalFileNameBytes();
    auto ret = 0;
    qDebug() << __FUNCTION__  << "(" << filename << ")";
    
    // Open file and make m_pFormatCtx
    if (( ret = avformat_open_input(&m_format_ctx, filename.constData(), nullptr,&l_format_opts))<0) {
        qDebug() << __FUNCTION__ << "cannot open" << filename << av_err2str ( ret );
        return ERR;
    }
    av_dict_free(&l_format_opts);
    // Retrieve stream information
    if ( ( ret = avformat_find_stream_info(m_format_ctx, nullptr)) < 0) {
        qDebug() << __FUNCTION__ << "cannot open" << filename << av_err2str ( ret );
        return ERR;
    }
    //debug only (Enable if needed)
    av_dump_format(m_format_ctx, 0, filename.constData(), false);
    for ( auto i = 0; i < m_format_ctx->nb_streams; i++)
    {
      m_format_ctx->streams[i]->discard = AVDISCARD_ALL;
    }
    if((m_stream_index = av_find_best_stream(
            m_format_ctx,
            AVMEDIA_TYPE_AUDIO,
            -1,
            -1,
            &m_codec,
            0))<0)
    {
      qDebug() << __FUNCTION__ << "cannot find audio stream in" << filename << av_err2str ( m_stream_index );
      return ERR;
    }
    m_stream          = m_format_ctx->streams[m_stream_index];
    m_stream->discard = AVDISCARD_NONE;
    if ( ! m_codec && ! ( m_codec = avcodec_find_decoder ( m_stream->codec->codec_id ) ) )
    {
      qDebug() << __FUNCTION__ << "cannot find codec for" << filename;
      return ERR;
    }
    if ( !(m_codec_ctx = avcodec_alloc_context3 ( m_codec )))
    {
      qDebug() << __FUNCTION__ << "cannot allocate codec context for" << m_codec->long_name;
      return ERR;
    }
    if ( ( ret = avcodec_copy_context ( m_codec_ctx, m_stream->codec ) ) < 0 )
    {
      qDebug() << __FUNCTION__ << "cannot copy avcodec context for " << filename << " with codec " << m_codec->long_name << av_err2str(ret);
      return ERR;
    };
    if((ret=av_dict_set(&l_format_opts,"refcounted_frames","1",0))<0)
    {
      qDebug() << __FUNCTION__ << "failed to set refcounted frames for " << filename << " with codec " << m_codec->long_name << av_err2str ( ret );
      return ERR;
    }

    if((ret = avcodec_open2(m_codec_ctx, m_codec,&l_format_opts))<0){
      qDebug() << __FUNCTION__ << "cannot open codec for" << filename << av_err2str ( ret );
      return ERR;
    }
    av_dict_free(&l_format_opts);
    m_stream_tb = m_stream->time_base;
    m_stream_tb_d = av_q2d ( m_stream_tb);
    m_codec_tb  = m_codec_ctx->time_base;
    m_codec_tb_d = av_q2d ( m_codec_tb );
    if ( config.channelCountHint <= 0 )
      setChannelCount( m_codec_ctx->channels );
    else
      setChannelCount( config.channelCountHint );
    setFrameRate ( m_codec_ctx->sample_rate );
    if(!(m_swr = swr_alloc ( )))
    {
      qDebug() << __FUNCTION__ << "cannot allocate swr context for" << filename << av_err2str(AVERROR(ENOMEM));
      return ERR;
    }
    if(!(m_orig_frame = av_frame_alloc () ) || !(m_frame = av_frame_alloc() ) ){
      qDebug() << __FUNCTION__ << "cannot allocate AVFrames for " << filename << av_err2str(AVERROR(ENOMEM));
      return ERR;
    }
    m_frame->channel_layout = av_get_default_channel_layout ( getChannelCount() );
    m_frame->format         = AV_SAMPLE_FMT_FLT;
    m_frame->sample_rate    = getFrameRate();
    av_init_packet ( &m_packet);
    m_packet.size = 0;
    m_packet.data = nullptr;
    auto discarded = size_t{0};
    do
    {
      ret = av_read_frame ( m_format_ctx, & m_packet );
      if ( ret < 0 || m_packet.stream_index != m_stream_index )
      {
        if ( m_packet.stream_index != m_stream_index )
        {
          discarded ++;
        }
        else if ( ret != AVERROR_EOF )
        {
          qDebug() << __FUNCTION__ << filename << "av_read_packet returned error" << av_err2str(ret);
        }
        av_free_packet(&m_packet);
      }
      else
      {
        av_dup_packet ( &m_packet );
        m_pkt_array.push_back ( m_packet );
        av_init_packet ( &m_packet );
      }
    }while ( ret >= 0 );
    if ( discarded )
    {
      qDebug() << __FUNCTION__ << "discarded" << discarded << "packets from other streams when demuxing" << filename;
    }
    if ( m_stream->start_time != AV_NOPTS_VALUE )
    {
      m_first_pts = m_stream->start_time;
    }
    else if ( m_pkt_array.size() )
    {
      m_first_pts = m_pkt_array.front().pts;
    }
    m_pkt_index = 0;
    if ( m_pkt_index < m_pkt_array.size() )
    {
      m_packet = m_pkt_array.at(m_pkt_index);
    }
    setFrameCount ( static_cast<SINT> ( (m_pkt_array.back().pts + m_pkt_array.back().duration) * ( m_stream_tb_d * getFrameRate () ) ) );
    qDebug() << __FUNCTION__ << ": frameRate = " << getFrameRate()
                         << ", channelCount = " << getChannelCount()
                         << ", frameCount = " << getFrameCount();
    m_frame->pts = AV_NOPTS_VALUE;
    return OK;
}
void SoundSourceFFmpeg::close() {
    swr_free ( &m_swr );
    while ( m_pkt_array.size() )
    {
      av_free_packet ( &m_pkt_array.back() );
      m_pkt_array.pop_back();
    }
    avcodec_close(m_codec_ctx);
    avcodec_free_context ( &m_codec_ctx );
    avformat_close_input(&m_format_ctx);
    av_frame_free(&m_orig_frame);
    av_frame_free(&m_frame     );
    m_stream       = nullptr;
    m_stream_index = -1;
    m_codec        = nullptr;
    m_pts          = AV_NOPTS_VALUE;
    m_first_pts    = AV_NOPTS_VALUE;
}

SINT SoundSourceFFmpeg::seekSampleFrame(SINT frameIndex) {
    DEBUG_ASSERT(isValidFrameIndex(frameIndex));
    if ( m_frame->pts == AV_NOPTS_VALUE && !decode_next_frame ( ) )
      return static_cast<int64_t> ( m_pts * m_stream_tb_d * getFrameRate ( ) );
    auto first_sample = static_cast<int64_t>(m_frame->pts * ( m_stream_tb_d *  getFrameRate ( ) ) );
    auto frame_pts = static_cast<int64_t>( frameIndex / ( m_stream_tb_d * getFrameRate() ) );
    if ( frame_pts >= m_frame->pts && frame_pts < m_frame->pts + static_cast<int64_t>(m_frame->nb_samples / ( m_stream_tb_d * getFrameRate() )  ))
    {
      m_pts = static_cast<int64_t>(frameIndex / ( m_stream_tb_d * getFrameRate() ) );
      return static_cast<int64_t> ( m_pts * m_stream_tb_d * getFrameRate ( ) );
    }
    if ( frame_pts <= m_pkt_array.front().pts )
    {
      m_pkt_index =  0;
      m_packet    =  m_pkt_array.at(m_pkt_index); 
      m_pts       =  m_packet.pts;
      m_offset    =  0;
      decode_next_frame ( );
      return static_cast<int64_t> ( m_frame->pts * ( m_stream_tb_d * getFrameRate ( ) ) ) + m_offset;;
    }
    if ( frame_pts >= m_pkt_array.back().pts )
    {
      m_pkt_index  = m_pkt_array.size() - 1;
      m_packet     = m_pkt_array.at ( m_pkt_index );
      decode_next_frame ( );
      m_offset     = frameIndex - static_cast<int64_t>(m_frame->pts * m_stream_tb_d * getFrameRate() );
      m_offset = std::max<int64_t>(0,std::min<int64_t>(m_frame->nb_samples-1,m_offset));
      return static_cast<int64_t> ( m_frame->pts * m_stream_tb_d * getFrameRate ( ) ) + m_offset;;
    }
    auto hindex = m_pkt_array.size() - 1;
    auto lindex = decltype(hindex){0};
    while ( hindex > lindex )
    {
      auto dist   = m_pkt_array.at(hindex).pts - m_pkt_array.at(lindex).pts;
      auto frac   = (frame_pts - m_pkt_array.at(lindex).pts)/static_cast<double>(dist);
      auto mindex = static_cast<decltype(hindex)>( frac * (hindex-lindex) ) + lindex;
      if ( mindex == lindex ) mindex++;
      if ( m_pkt_array.at(mindex).pts > frame_pts) 
        hindex = mindex;
      else if ( m_pkt_array.at(mindex).pts + m_pkt_array.at(mindex).duration <= frame_pts )
        lindex = mindex;
      else
      {
        m_pkt_index = mindex;
        break;
      }
    }
    if ( hindex == lindex ) m_pkt_index = lindex;
    if ( m_pkt_index >= m_pkt_array.size ( ) )
      m_pkt_index = m_pkt_array.size() - 1;
    m_packet = m_pkt_array.at ( m_pkt_index );
    if ( swr_is_initialized ( m_swr ) )
    {
      auto delay = swr_get_delay ( m_swr, getFrameRate ( ) );
      if ( delay > 0 ) swr_drop_output ( m_swr, delay);
    }
    decode_next_frame ( );
    m_offset = static_cast<int64_t>((frame_pts - m_frame->pts ) * m_stream_tb_d * getFrameRate () );
    return static_cast<int64_t>(m_frame->pts * ( m_stream_tb_d * getFrameRate() ) ) + m_offset;;
}

bool SoundSourceFFmpeg::decode_next_frame(){
  auto ret = 0;
  auto decoding_errors = 0;
  while ( true )
  {
    if ( m_packet.size <= 0 )
    {
      m_pkt_index++;
      if ( m_pkt_index >= m_pkt_array.size() )
      {
        return false;
      }
      else
      {
        m_packet = m_pkt_array.at(m_pkt_index);
      }
    }
    else
    {
      auto got_frame = int{0};
      if ( m_orig_frame )
        av_frame_unref ( m_orig_frame );
      else if(!(m_orig_frame=av_frame_alloc ( ) ) ) 
        return false;
      ret = avcodec_decode_audio4 ( m_codec_ctx, m_orig_frame, &got_frame, &m_packet );
      if ( ret < 0 )
      {
        qDebug() << __FUNCTION__ << "error decoding packet" << m_pkt_index << "for" << getLocalFileName() << av_err2str ( ret );
        av_init_packet ( &m_packet );
        m_packet.size = 0;
        m_packet.data = nullptr;
        decoding_errors++;
        if ( decoding_errors > 8 )
        {
          qDebug () << __FUNCTION__ << "too many decoding errors for" << getLocalFileName() << "aborting decode";
          break;
        }
      }
      else
      {
        if ( ret == 0 )
        {
          ret = m_packet.size;
        }
        m_packet.size -= ret;
        m_packet.data += ret;
        if ( got_frame )
        {
          if ( m_orig_frame->pts == AV_NOPTS_VALUE )
            m_orig_frame->pts = av_frame_get_best_effort_timestamp ( m_orig_frame );
          if ( m_frame ) av_frame_unref ( m_frame );
          else if ( !( m_frame = av_frame_alloc ( ) ) ) return false;
          m_frame->format         = AV_SAMPLE_FMT_FLT;
          m_frame->channel_layout = av_get_default_channel_layout ( getChannelCount() );
          m_frame->sample_rate    = getFrameRate();
          if ( !swr_is_initialized ( m_swr ) )
          {
            if ( (ret = swr_config_frame ( m_swr, m_frame, m_orig_frame ) ) < 0 
              || (ret = swr_init ( m_swr ) ) < 0 )
            {
              qDebug() << __FUNCTION__ << "error initializing SwrContext" << av_err2str ( ret );
            }
          }
          auto delay = swr_get_delay ( m_swr, getFrameRate() );
          {
            m_frame->pts     = m_orig_frame->pts - static_cast<int64_t>(delay / ( m_stream_tb_d * getFrameRate() ) );
            m_frame->pkt_pts = m_orig_frame->pkt_pts;
            m_frame->pkt_dts = m_orig_frame->pkt_dts;
          }
          if ( ( ret = swr_convert_frame ( m_swr, m_frame, m_orig_frame ) ) < 0 )
          {
            if ( ( ret = swr_config_frame ( m_swr, m_frame, m_orig_frame ) ) < 0 
              || ( ret = swr_convert_frame( m_swr, m_frame, m_orig_frame ) ) < 0 )
            {
              qDebug() << __FUNCTION__ << "error converting frame for" << getLocalFileName()<< av_err2str(ret);
              continue;
            }
          }
          return true;
        }
      }
    }
  }
  return false;
}

SINT SoundSourceFFmpeg::readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer) {
    auto number_done   = decltype(numberOfFrames){0};
    uint8_t *dst[] = {reinterpret_cast<uint8_t*>(sampleBuffer)};
    while ( number_done < numberOfFrames)
    {
      if ( m_frame->nb_samples == 0 || m_offset >= m_frame->nb_samples )
      {
        m_offset -= m_frame->nb_samples;
        if ( !decode_next_frame() ) break;
      }
      else
      {
        auto this_chunk = std::min<int64_t>(m_frame->nb_samples - m_offset, numberOfFrames - number_done );
        if ( sampleBuffer )
        {
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
        number_done = this_chunk;
        m_offset   += this_chunk;
        m_pts += static_cast<int64_t>(this_chunk / ( m_stream_tb_d * getFrameRate()) );
      }
    }
    return number_done;
}
} // namespace Mixxx
