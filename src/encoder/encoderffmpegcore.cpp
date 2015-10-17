//
//   FFMPEG encoder class..
//     - Supports what FFMPEG is compiled to supported
//     - Same interface for all codecs
//

#include "encoder/encoderffmpegcore.h"

#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <QtDebug>

#include "encoder/encodercallback.h"
#include "preferences/errordialoghandler.h"
//
// FFMPEG changed their variable/define names in 1.0
// smallest number that is AV_/AV compatible avcodec version is
// 54/59/100 which is 3554148
//
// Constructor
EncoderFfmpegCore::EncoderFfmpegCore(EncoderCallback* pCallback, AVCodecID codec)
   : m_pCallback(pCallback), m_SCcodecId(codec)
{
    m_lBitrate = 128000;
}
// Destructor  //call flush before any encoder gets deleted
EncoderFfmpegCore::~EncoderFfmpegCore()
{
    qDebug() << "EncoderFfmpegCore::~EncoderFfmpegCore()";
    closeAudio();
}
//call sendPackages() or write() after 'flush()' as outlined in engineshoutcast.cpp
void EncoderFfmpegCore::flush() 
{
  writeAudioFrames(false);
}
//  Get new random serial number
//  -> returns random number
int EncoderFfmpegCore::getSerial()
{
    auto l_iSerial = 0;
    return l_iSerial;
}
void EncoderFfmpegCore::encodeBuffer(const CSAMPLE *samples, const int size)
{
    auto pre_frame = av_frame_alloc();
    pre_frame->nb_samples     = size;
    pre_frame->format         = AV_SAMPLE_FMT_FLT;
    pre_frame->channels       = 2;
    pre_frame->sample_rate    = 44100;
    pre_frame->channel_layout = av_get_default_channel_layout(pre_frame->channels);
    auto buffer_size = av_samples_get_buffer_size(nullptr,pre_frame->channels,size,(AVSampleFormat)pre_frame->format,0);
    auto err = avcodec_fill_audio_frame(pre_frame,pre_frame->channels,(AVSampleFormat)pre_frame->format,reinterpret_cast<const uint8_t*>(samples),buffer_size,0);
    auto post_frame = av_frame_alloc();
    post_frame->format         = m_pEncoderCodecCtx->sample_fmt;
    post_frame->channels       = m_pEncoderCodecCtx->channels;
    post_frame->channel_layout = m_pEncoderCodecCtx->channel_layout;
    post_frame->sample_rate    = m_pEncoderCodecCtx->sample_rate;
    if((err=swr_convert_frame(m_pSwr,post_frame,pre_frame))<0)
    {
      if(((err=swr_config_frame(m_pSwr,post_frame,pre_frame))<0)
      || ((err=swr_convert_frame(m_pSwr,post_frame,pre_frame))<0))
      {
        av_frame_free(&pre_frame);
        av_frame_free(&post_frame);
        return ;
      }
    }
    av_frame_free(&pre_frame);
    av_audio_fifo_write(m_pAudioFifo,reinterpret_cast<void**>(post_frame->extended_data),post_frame->nb_samples);
    av_frame_free(&post_frame);
    writeAudioFrames(false);
}
// Originally called from engineshoutcast.cpp to update metadata information
// when streaming, however, this causes pops
//
// Currently this method is used before init() once to save artist, title and album
//
void EncoderFfmpegCore::updateMetaData(char* artist, char* title, char* album)
{
    qDebug() << "ffmpegencodercore: UpdateMetadata: !" << artist << " - " << title << " - " << album;
    m_strMetaDataTitle = title;
    m_strMetaDataArtist = artist;
    m_strMetaDataAlbum = album;
}
int EncoderFfmpegCore::initEncoder(int bitrate, int samplerate)
{
    m_lBitrate = bitrate * 1000;
    m_lSampleRate = samplerate;
    const char *output_name = nullptr;
    if (m_SCcodecId == AV_CODEC_ID_MP3)      output_name = "output.mp3";
    else if (m_SCcodecId == AV_CODEC_ID_AAC) output_name = "output.m4a";
    else                                     output_name = "output.ogg";
    m_pEncoderFormatCtx = avformat_alloc_context();
    auto buffer = reinterpret_cast<uint8_t*>(av_malloc(4096));
    m_pEncoderFormatCtx->pb = avio_alloc_context(buffer,4096,1,reinterpret_cast<void*>(m_pCallback),nullptr,&EncoderCallback::write_thunk,nullptr);
    m_pEncoderFormatCtx->oformat = av_guess_format(nullptr,output_name,nullptr);
    avformat_alloc_output_context2(&m_pEncoderFormatCtx,nullptr,nullptr,output_name);
    m_pEncoderFormatCtx->oformat->audio_codec = m_SCcodecId;
    m_pEncoderFormat                   = m_pEncoderFormatCtx->oformat;
    m_pEncoderAudioCodec               = avcodec_find_encoder(m_pEncoderFormat->audio_codec);
    m_pEncoderAudioStream              = avformat_new_stream(m_pEncoderFormatCtx,m_pEncoderAudioCodec);

    m_pEncoderCodecCtx                 = m_pEncoderAudioStream->codec;
    m_pEncoderCodecCtx->bit_rate       = m_lBitrate;
    m_pEncoderCodecCtx->sample_rate    = 44100;
    m_pEncoderCodecCtx->channels       = 2;
    m_pEncoderCodecCtx->channel_layout = av_get_default_channel_layout(m_pEncoderCodecCtx->channels);
    m_pEncoderCodecCtx->sample_fmt     = m_pEncoderAudioCodec->sample_fmts[0];
    m_pEncoderCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    if(m_pEncoderFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) m_pEncoderCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    avcodec_open2(m_pEncoderCodecCtx,m_pEncoderAudioCodec,nullptr);
    m_pSwr = swr_alloc_set_opts(
        m_pSwr,
        m_pEncoderCodecCtx->channel_layout,
        m_pEncoderCodecCtx->sample_fmt,
        m_pEncoderCodecCtx->sample_rate,
        av_get_default_channel_layout(2),
        AV_SAMPLE_FMT_FLT,
        44100,
        0, nullptr);
    swr_init(m_pSwr);
    m_pAudioFifo = av_audio_fifo_alloc(m_pEncoderCodecCtx->sample_fmt,m_pEncoderCodecCtx->channels,1);
    if (avformat_write_header(m_pEncoderFormatCtx, nullptr))
    {
        qDebug() << "EncoderFfmpegCore::initEncoder: failed to write a header.";
        return -1;
    }
    if(!(m_iAudioInputFrameSize = m_pEncoderCodecCtx->frame_size)) m_iAudioInputFrameSize = 1024;
    m_bStreamInitialized = true;
    // qDebug() << "jepusti";
    return 0;
}
// Private methods
int EncoderFfmpegCore::writeAudioFrames(bool flushing)
{
    auto got_frame = 0;
    auto err = 0;
    // Mixxx uses float (32 bit) samples..
    auto frame = av_frame_alloc();
    AVPacket packet;
    while(av_audio_fifo_size(m_pAudioFifo) >= m_iAudioInputFrameSize || (av_audio_fifo_size(m_pAudioFifo) > 0 && flushing))
    {
      av_init_packet(&packet);
      packet.size = 0;
      packet.data = nullptr;
      // Calculate correct DTS for FFMPEG
      m_lDts = round(((double)m_lRecordedBytes / (double)44100 / (double)2. * (double)m_pEncoderAudioStream->time_base.den));
      m_lPts = m_lDts;

      frame->nb_samples     = m_iAudioInputFrameSize;
      frame->format         = m_pEncoderCodecCtx->sample_fmt;
      frame->channel_layout = m_pEncoderCodecCtx->channel_layout;
      frame->channels       = m_pEncoderCodecCtx->channels;
      frame->sample_rate    = m_pEncoderCodecCtx->sample_rate;
      err = av_frame_get_buffer ( frame, 0 );
      if (err)
      {
          qDebug() << "Can't fill FFMPEG frame: error " <<
                  ff_make_error_string(err) << " " << "m_iAudioInputFrameSize= " <<  m_iAudioInputFrameSize;
          av_frame_free(&frame);
          return -1;
      }
      frame->nb_samples = av_audio_fifo_read(m_pAudioFifo,reinterpret_cast<void**>(frame->extended_data),frame->nb_samples);
      frame->pts = m_lPts;
      avcodec_encode_audio2(m_pEncoderCodecCtx,&packet,frame,&got_frame);
      av_frame_unref(frame);
      packet.stream_index = m_pEncoderAudioStream->index;
      packet.dts          = m_lDts;
      packet.pts          = m_lPts;
      if((err = av_interleaved_write_frame(m_pEncoderFormatCtx,&packet))<0)
      {
        qDebug() << "Error while writing audio frame " << ff_make_error_string(err);
      }
      av_free_packet(&packet);
    }
    av_frame_free(&frame);
    return 0;
}
void EncoderFfmpegCore::closeAudio()
{
    if(m_bStreamInitialized)
    {
      writeAudioFrames(true);
      av_write_trailer(m_pEncoderFormatCtx);
    }
    swr_free(&m_pSwr);
    avcodec_close(m_pEncoderCodecCtx);
    if(m_pEncoderFormatCtx->pb)
    {
      av_freep(&m_pEncoderFormatCtx->pb->buffer);
      av_freep(&m_pEncoderFormatCtx->pb);
    }
    avformat_free_context(m_pEncoderFormatCtx);
}

