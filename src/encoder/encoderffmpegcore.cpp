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
#include "errordialoghandler.h"
//
// FFMPEG changed their variable/define names in 1.0
// smallest number that is AV_/AV compatible avcodec version is
// 54/59/100 which is 3554148
//
// Constructor
namespace{
  QString ff_make_error_string(int err)
  {
    char str[256];
    av_strerror(err,str,sizeof(str));
    return QString{str};
  }
};
EncoderFfmpegCore::EncoderFfmpegCore(EncoderCallback* pCallback, AVCodecID codec)
   : m_pCallback(pCallback),
     m_SCcodecId(codec)
{
    m_lBitrate = 128000;
}
// Destructor  //call flush before any encoder gets deleted
EncoderFfmpegCore::~EncoderFfmpegCore() {
    qDebug() << "EncoderFfmpegCore::~EncoderFfmpegCore()";
    av_freep(&m_pSamples);
    av_freep(&m_pFltSamples);
    avio_open_dyn_buf(&m_pEncodeFormatCtx->pb);
    if (av_write_trailer(m_pEncodeFormatCtx) != 0) {
        qDebug() << "Multiplexer: failed to write a trailer.";
    } else {
        unsigned char *l_strBuffer = nullptr;
        int l_iBufferLen = 0;
        l_iBufferLen = avio_close_dyn_buf(m_pEncodeFormatCtx->pb, (uint8_t**)(&l_strBuffer));
        m_pCallback->write(nullptr, l_strBuffer, 0, l_iBufferLen);
        av_free(l_strBuffer);
    }
    if (m_pStream != nullptr) { avcodec_close(m_pStream->codec);}
    if (m_pEncodeFormatCtx != nullptr) { av_free(m_pEncodeFormatCtx); }
    // Close buffer
    swr_free(&m_swr);
}
//call sendPackages() or write() after 'flush()' as outlined in engineshoutcast.cpp
void EncoderFfmpegCore::flush() {}
//  Get new random serial number
//  -> returns random number
int EncoderFfmpegCore::getSerial() {
    int l_iSerial = 0;
    return l_iSerial;
}
void EncoderFfmpegCore::encodeBuffer(const CSAMPLE *samples, const int size) {
    unsigned char *l_strBuffer = nullptr;
    int l_iBufferLen = 0;
    //int l_iAudioCpyLen = m_iAudioInputFrameSize *
    //                     av_get_bytes_per_sample(m_pEncoderAudioStream->codec->sample_fmt) *
    //                     m_pEncoderAudioStream->codec->channels;
    long l_iLeft = size;
    long j = 0;
    unsigned int l_iBufPos = 0;
    unsigned int l_iPos = 0;
    // In MP3 this writes Header same In ogg
    // They are written once front of the encoded stuff
    if (m_bStreamInitialized == false) {
        m_bStreamInitialized = true;
        // Write a header.
        avio_open_dyn_buf(&m_pEncodeFormatCtx->pb);
        if (avformat_write_header(m_pEncodeFormatCtx, nullptr) != 0) {
            qDebug() << "EncoderFfmpegCore::encodeBuffer: failed to write a header.";
            return;
        }
        l_iBufferLen = avio_close_dyn_buf(m_pEncodeFormatCtx->pb, (uint8_t**)(&l_strBuffer));
        m_pCallback->write(nullptr, l_strBuffer, 0, l_iBufferLen);
        av_freep(&l_strBuffer);
    }
    while (l_iLeft > (m_iFltAudioCpyLen / 4)) {
        memset(m_pFltSamples, 0x00, m_iFltAudioCpyLen);
        for (j = 0; j < m_iFltAudioCpyLen / 4; j++) {
            if (m_lBufferSize > 0) {
                m_pFltSamples[j] = m_SBuffer[ l_iBufPos++ ];
                m_lBufferSize--;
                m_lRecordedBytes++;
            } else {
                m_pFltSamples[j] = samples[l_iPos++];
                l_iLeft--;
                m_lRecordedBytes++;
            }
            if (l_iLeft <= 0) {
                qDebug() << "ffmpegencodercore: No samples left.. for encoding!";
                break;
            }
        }
        m_lBufferSize = 0;
        // Open dynamic buffer for writing next bytes
        if (avio_open_dyn_buf(&m_pEncodeFormatCtx->pb) < 0) {
            qDebug() << "Can't alloc Dyn buffer!";
            return;
        }
        // Write it to buffer (FILE) and then close buffer for waiting
        // Next encoded buffe to come or we stop encode
        if (! writeAudioFrame(m_pEncodeFormatCtx, m_pEncoderAudioStream)) {
            l_iBufferLen = avio_close_dyn_buf(m_pEncodeFormatCtx->pb,(uint8_t**)(&l_strBuffer));
            m_pCallback->write(nullptr, l_strBuffer, 0, l_iBufferLen);
            av_freep(&l_strBuffer);
        }
    }
    // Keep things clean
    std::memset(m_SBuffer, 0x00, 65535);
    for (j = 0; j < l_iLeft; j++) { m_SBuffer[ j ] = samples[ l_iPos++ ]; }
    m_lBufferSize = l_iLeft;
}

// Originally called from engineshoutcast.cpp to update metadata information
// when streaming, however, this causes pops
//
// Currently this method is used before init() once to save artist, title and album
//
void EncoderFfmpegCore::updateMetaData(char* artist, char* title, char* album) {
    qDebug() << "ffmpegencodercore: UpdateMetadata: !" << artist << " - " << title << " - " << album;
    m_strMetaDataTitle = title;
    m_strMetaDataArtist = artist;
    m_strMetaDataAlbum = album;
}
int EncoderFfmpegCore::initEncoder(int bitrate, int samplerate) {
#ifndef avformat_alloc_output_context2
    qDebug() << "EncoderFfmpegCore::initEncoder: Old Style initialization";
    m_pEncodeFormatCtx = avformat_alloc_context();
#endif
    m_lBitrate = bitrate * 1000;
    m_lSampleRate = samplerate;
    if (m_SCcodecId == AV_CODEC_ID_MP3) {
        qDebug() << "EncoderFfmpegCore::initEncoder: Codec MP3";
#ifdef avformat_alloc_output_context2
        avformat_alloc_output_context2(&m_pEncodeFormatCtx, nullptr, nullptr, "output.mp3");
#else
        m_pEncoderFormat = av_guess_format(nullptr, "output.mp3", nullptr);
#endif // avformat_alloc_output_context2
    } else if (m_SCcodecId == AV_CODEC_ID_AAC) {
        qDebug() << "EncoderFfmpegCore::initEncoder: Codec M4A";
#ifdef avformat_alloc_output_context2
        avformat_alloc_output_context2(&m_pEncodeFormatCtx, nullptr, nullptr, "output.m4a");
#else
        m_pEncoderFormat = av_guess_format(nullptr, "output.m4a", nullptr);
#endif // avformat_alloc_output_context2
    } else {
        qDebug() << "EncoderFfmpegCore::initEncoder: Codec OGG/Vorbis";
#ifdef avformat_alloc_output_context2
        avformat_alloc_output_context2(&m_pEncodeFormatCtx, nullptr, nullptr, "output.ogg");
        m_pEncodeFormatCtx->oformat->audio_codec=AV_CODEC_ID_VORBIS;
#else
        m_pEncoderFormat = av_guess_format(nullptr, "output.ogg", nullptr);
        m_pEncoderFormat->audio_codec=AV_CODEC_ID_VORBIS;
#endif // avformat_alloc_output_context2
    }
#ifdef avformat_alloc_output_context2
    m_pEncoderFormat = m_pEncodeFormatCtx->oformat;
#else
    m_pEncodeFormatCtx->oformat = m_pEncoderFormat;
#endif // avformat_alloc_output_context2
    m_pEncoderAudioStream = addStream(m_pEncodeFormatCtx, &m_pEncoderAudioCodec,m_pEncoderFormat->audio_codec);
    openAudio(m_pEncoderAudioCodec, m_pEncoderAudioStream);
    // qDebug() << "jepusti";
    return 0;
}
// Private methods
int EncoderFfmpegCore::writeAudioFrame(AVFormatContext *formatctx, AVStream *stream) {
    AVCodecContext *l_SCodecCtx = nullptr;;
    AVPacket l_SPacket;
    int l_iGotPacket;
    int l_iRet;
    av_init_packet(&l_SPacket);
    l_SPacket.size = 0;
    l_SPacket.data = nullptr;
    // Calculate correct DTS for FFMPEG
    m_lDts = round(((double)m_lRecordedBytes / (double)44100 / (double)2. * (double)m_pEncoderAudioStream->time_base.den));
    m_lPts = m_lDts;
    l_SCodecCtx = stream->codec;
    // Mixxx uses float (32 bit) samples..
    auto l_SFrame = av_frame_alloc();
    av_frame_unref(l_SFrame);
    l_SFrame->nb_samples     = m_iAudioInputFrameSize;
    l_SFrame->format         = l_SCodecCtx->sample_fmt;
    l_SFrame->channel_layout = l_SCodecCtx->channel_layout;
    l_SFrame->channels       = l_SCodecCtx->channels;
    l_SFrame->sample_rate    = l_SCodecCtx->sample_rate;
    l_iRet = av_frame_get_buffer ( l_SFrame, 0 );
    if (l_iRet != 0) {
        qDebug() << "Can't fill FFMPEG frame: error " <<
                 ff_make_error_string(l_iRet) << " " << "m_iAudioInputFrameSize= " <<  m_iAudioInputFrameSize;
        qDebug() << "Can't refill 1st FFMPEG frame!";
        av_frame_free(&l_SFrame);
        return -1;
    }
    // If we have something else than AV_SAMPLE_FMT_FLT we have to convert it
    // to something that fits..
    {
      auto src =  (uint8_t*)m_pFltSamples;
      l_iRet = swr_convert(m_swr,l_SFrame->extended_data,l_SFrame->nb_samples, const_cast<const uint8_t**>(&src), m_iFltAudioCpyLen);
    }
    // After we have turned our samples to destination
    // Format we must re-alloc l_SFrame.. it easier like this..
    // FFMPEG 2.2 3561060 anb beyond
/*    av_frame_unref(l_SFrame);
    l_SFrame->nb_samples     = m_iAudioInputFrameSize;
    l_SFrame->format         = l_SCodecCtx->sample_fmt;
    l_SFrame->channel_layout = m_pEncoderAudioStream->codec->channel_layout;
    l_iRet = av_frame_get_buffer ( l_SFrame, 16 );
    free(l_iOut);
    l_iOut = nullptr;*/
    if (l_iRet < 0) {
        qDebug() << "Can't refill FFMPEG frame: error " << l_iRet << "String '" <<
                  ff_make_error_string(l_iRet) << "'" <<  m_iAudioCpyLen <<
                  " " <<  av_samples_get_buffer_size(
                      nullptr, 2,
                      m_iAudioInputFrameSize,
                      m_pEncoderAudioStream->codec->sample_fmt,
                      1) ;
        qDebug() << "Can't refill 2nd FFMPEG frame!"  ;
        av_frame_free(&l_SFrame);
        return -1;
    }
    else
    {
      l_SFrame->nb_samples = l_iRet;
    }
    //qDebug() << "!!" << l_iRet;
    l_iRet = avcodec_encode_audio2(l_SCodecCtx, &l_SPacket, l_SFrame, &l_iGotPacket);
    if (l_iRet < 0) {
        qDebug() << "Error encoding audio frame" << ff_make_error_string(l_iRet);
        av_frame_free(&l_SFrame);
        return -1;
    }
    if (!l_iGotPacket) {
      av_frame_free(&l_SFrame);
      return 0;
    }
    l_SPacket.stream_index = stream->index;
    // Let's calculate DTS/PTS and give it to FFMPEG..
    // THEN codecs like OGG/Voris works ok!!
    l_SPacket.dts = m_lDts;
    l_SPacket.pts = m_lDts;
    // Write the compressed frame to the media file. */
    l_iRet = av_interleaved_write_frame(formatctx, &l_SPacket);
    if (l_iRet < 0) {
        qDebug() << "Error while writing audio frame" << ff_make_error_string(l_iRet);
        return -1;
    }
    av_free_packet(&l_SPacket);
    av_frame_free(&l_SFrame);
    return 0;
}
void EncoderFfmpegCore::closeAudio(AVStream *stream) {
    avcodec_close(stream->codec);
    av_free(m_pSamples);
}
void EncoderFfmpegCore::openAudio(AVCodec *codec, AVStream *stream) {
    AVCodecContext *l_SCodecCtx;
    int l_iRet;
    l_SCodecCtx = stream->codec;
    qDebug() << "openCodec!";
    // open it
    l_iRet = avcodec_open2(l_SCodecCtx, codec, nullptr);
    if (l_iRet < 0) {
        qDebug() << "Could not open audio codec!";
        return;
    }
    if (l_SCodecCtx->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE) {m_iAudioInputFrameSize = 10000;}
    else { m_iAudioInputFrameSize = l_SCodecCtx->frame_size;}
    m_iAudioCpyLen = m_iAudioInputFrameSize * av_get_bytes_per_sample(stream->codec->sample_fmt) * stream->codec->channels;
    m_iFltAudioCpyLen = av_samples_get_buffer_size(nullptr, 2, m_iAudioInputFrameSize, AV_SAMPLE_FMT_FLT,1);
    // m_pSamples is destination samples.. m_pFltSamples is FLOAT (32 bit) samples..
    m_pSamples = (uint8_t *)av_malloc(m_iAudioCpyLen * sizeof(uint8_t));
    //m_pFltSamples = (uint16_t *)av_malloc(m_iFltAudioCpyLen);
    m_pFltSamples = (float *)av_malloc(m_iFltAudioCpyLen * sizeof(float));
    if (!m_pSamples) {
        qDebug() << "Could not allocate audio samples buffer";
        return;
    }
}
// Add an output stream.
AVStream *EncoderFfmpegCore::addStream(AVFormatContext *formatctx, AVCodec **codec, enum AVCodecID codec_id) {
    AVCodecContext *l_SCodecCtx = nullptr;
    AVStream *l_SStream = nullptr;
    // find the encoder
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        return nullptr;
    }
    l_SStream = avformat_new_stream(formatctx, *codec);
    if (!l_SStream) {
        qDebug() << "Could not allocate stream";
        return nullptr;
    }
    l_SStream->id = formatctx->nb_streams-1;
    l_SCodecCtx = l_SStream->codec;
    m_swr = swr_alloc_set_opts(m_swr,
      av_get_default_channel_layout(2),
      m_pEncoderAudioCodec->sample_fmts[0],
      44100,
      av_get_default_channel_layout(2),
      AV_SAMPLE_FMT_FLT,
      44100,
      0,
      nullptr
      );
    swr_init(m_swr);
    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        l_SStream->id = 1;
        l_SCodecCtx->sample_fmt = m_pEncoderAudioCodec->sample_fmts[0];
        l_SCodecCtx->bit_rate    = m_lBitrate;
        l_SCodecCtx->sample_rate = 44100;
        l_SCodecCtx->channels    = 2;
        l_SCodecCtx->channel_layout = av_get_default_channel_layout(2);
        break;
    default: break;
    }
    // Some formats want stream headers to be separate.
    if (formatctx->oformat->flags & AVFMT_GLOBALHEADER) l_SCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    return l_SStream;
}
