//
//   FFMPEG encoder class..
//     - Supports what FFMPEG is compiled to supported
//     - Same interface for all codecs
//

#include "encoder/encoderffmpegcore.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <QtDebug>

#include "encoder/encodercallback.h"
#include "errordialoghandler.h"
//
// FFMPEG changed their variable/define names in 1.0
// smallest number that is AV_/AV compatible avcodec version is
// 54/59/100 which is 3554148
//
// Constructor
EncoderFfmpegCore::EncoderFfmpegCore(
    EncoderCallback* pCallback
  , AVCodecID codec
  , const char *example_filename)
: m_callback(pCallback)
, m_codec_id(codec)
, m_codec(avcodec_find_encoder(codec))
, m_codec_ctx(m_codec)
, m_output_fmt(av_guess_format(nullptr, example_filename,nullptr))
{ }

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
        auto l_iBufferLen = 0;
        l_iBufferLen = avio_close_dyn_buf(m_pEncodeFormatCtx->pb,(uint8_t**)(&l_strBuffer));
        m_pCallback->write(nullptr, l_strBuffer, 0, l_iBufferLen);
        av_freep(&l_strBuffer);
    }
    if (m_pStream) {
        avcodec_close(m_pStream->codec);
    }
    if (m_pEncodeFormatCtx) {
        avformat_free_context(m_pEncodeFormatCtx);
        m_pEncodeFormatCtx = nullptr;
    }
    // Close buffer
    delete m_pResample;
}
//call sendPackages() or write() after 'flush()' as outlined in enginebroadcast.cpp
void EncoderFfmpegCore::flush() { }

//  Get new random serial number
//  -> returns random number

int EncoderFfmpegCore::getSerial()
{
    int l_iSerial = 0;
    return l_iSerial;
}
void EncoderFfmpegCore::encodeBuffer(const CSAMPLE *samples, const int size)
{
    unsigned char *l_strBuffer = NULL;
    int l_iBufferLen = 0;
    //int l_iAudioCpyLen = m_iAudioInputFrameSize *
    //                     av_get_bytes_per_sample(m_pEncoderAudioStream->codec->sample_fmt) *
    //                     m_pEncoderAudioStream->codec->channels;
    long l_iLeft = size;
    long j = 0;
    unsigned int l_iBufPos = 0;
    unsigned int l_iPos = 0;

    // TODO(XXX): Get rid of repeated malloc here!
    float *l_fNormalizedSamples = (float *)malloc(size * sizeof(float));

    // We use normalized floats in the engine [-1.0, 1.0] and FFMPEG expects
    // samples in the range [-1.0, 1.0] so no conversion is required.
    for (j = 0; j < size; j++) {
        l_fNormalizedSamples[j] = samples[j];
    }

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
        l_iBufferLen = avio_close_dyn_buf(m_pEncodeFormatCtx->pb,(uint8_t**)(&l_strBuffer));
        m_pCallback->write(l_strBuffer,nullptr, l_iBufferLen,0);
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
                m_pFltSamples[j] = l_fNormalizedSamples[l_iPos++];
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
            av_free(l_strBuffer);
        }
    }
    // Keep things clean
    memset(m_SBuffer, 0x00, 65535);

    for (j = 0; j < l_iLeft; j++) {
        m_SBuffer[ j ] = l_fNormalizedSamples[ l_iPos++ ];
    }
    m_lBufferSize = l_iLeft;
    free(l_fNormalizedSamples);
}
// Originally called from enginebroadcast.cpp to update metadata information
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

// Private methods
int EncoderFfmpegCore::writeAudioFrame(const CSAMPLE *samples, int size)
{

    // Calculate correct DTS for FFMPEG
    m_lDts = round(((double)m_lRecordedBytes / (double)44100 / (double)2. *
                    (double)m_pEncoderAudioStream->time_base.den));
    m_lPts = m_lDts;

    l_SCodecCtx = stream->codec;

    l_SFrame->nb_samples = m_iAudioInputFrameSize;
    // Mixxx uses float (32 bit) samples..
    l_SFrame->format = AV_SAMPLE_FMT_FLT;
    l_SFrame->channel_layout = l_SCodecCtx->channel_layout;
    l_iRet = avcodec_fill_audio_frame(l_SFrame,
                                      l_SCodecCtx->channels,
                                      AV_SAMPLE_FMT_FLT,
                                      (const uint8_t *)m_pFltSamples,
                                      m_iFltAudioCpyLen,
                                      1);

    if (l_iRet != 0) {
        qDebug() << "Can't fill FFMPEG frame: error "
                 << l_iRet
                 << "String '"
                 << mixxx::av_strerror(l_iRet)
                 << "'" << m_iFltAudioCpyLen;
        qDebug() << "Can't refill 1st FFMPEG frame!";
        return -1;
    }

    // If we have something else than AV_SAMPLE_FMT_FLT we have to convert it
    // to something that fits..
    if (l_SCodecCtx->sample_fmt != AV_SAMPLE_FMT_FLT)
    {
        m_pResample->reSampleMixxx(l_SFrame, &l_iOut);
        // After we have turned our samples to destination
        // Format we must re-alloc l_SFrame.. it easier like this..
        // FFMPEG 2.2 3561060 anb beyond
        av_frame_free(&l_SFrame);
        l_SFrame = av_frame_alloc();
        l_SFrame->nb_samples = m_iAudioInputFrameSize;
        l_SFrame->format = l_SCodecCtx->sample_fmt;
        l_SFrame->channel_layout = m_pEncoderAudioStream->codec->channel_layout;
        l_iRet = avcodec_fill_audio_frame(l_SFrame, l_SCodecCtx->channels,
                                          l_SCodecCtx->sample_fmt,
                                          l_iOut,
                                          m_iAudioCpyLen,
                                          1);
        free(l_iOut);
        l_iOut = NULL;

        if (l_iRet != 0) {
            qDebug()
                << "Can't refill FFMPEG frame: error "
                << l_iRet
                << "String '"
                << av_strerror(l_iRet)
                << "'" <<  m_iAudioCpyLen
                <<" "
                <<  av_samples_get_buffer_size(
                         nullptr, 2,
                         m_iAudioInputFrameSize,
                         m_pEncoderAudioStream->codec->sample_fmt,
                         1)
                << " " << m_pOutSize;
            qDebug() << "Can't refill 2nd FFMPEG frame!";
            return -1;
        }
    }
    //qDebug() << "!!" << l_iRet;
    l_iRet = avcodec_encode_audio2(l_SCodecCtx, l_SPacket, l_SFrame,&l_iGotPacket);
    if (l_iRet < 0) {
        qDebug() << "Error encoding audio frame";
        return -1;
    }
    if (!l_iGotPacket) {
        // qDebug() << "No packet! Can't encode audio!!";
        return -1;
    }
    l_SPacket->stream_index = stream->index;
    // Let's calculate DTS/PTS and give it to FFMPEG..
    // THEN codecs like OGG/Voris works ok!!
    l_SPacket->dts = m_lDts;
    l_SPacket->pts = m_lDts;
    // Write the compressed frame to the media file. */
    l_iRet = av_interleaved_write_frame(formatctx, l_SPacket);
    if (l_iRet != 0) {
        qDebug() << "Error while writing audio frame";
        return -1;
    }
    av_packet_free(&l_SPacket);
    av_frame_free(&l_SFrame);
    return 0;
}

int EncoderFfmpegCore::initEncoder(int bitrate, int samplerate)
{
    m_bitrate = bitrate * 1000;
    m_sampleRate = samplerate;

    m_format_ctx->oformat = m_output_fmt;
    m_stream     = m_format_ctx.new_stream(nullptr);
    m_stream->id = m_format_ctx->nb_streams - 1;

    m_codec_ctx->channels = 2;
    m_codec_ctx->channel_layout = av_get_default_channel_layout(m_codec_ctx->channels);
    m_codec_ctx->sample_rate = samplerate;
    m_codec_ctx->bit_rate = m_bitrate;
    m_codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    m_codec_ctx->sample_fmt = m_codec->sample_fmts[0];

    m_stream->time_base.den = samplerate;
    m_stream->time_base.num = 1;

    if(m_foramt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        m_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    auto err = 0;
    if((err = m_codec_ctx.open(m_codec)) < 0)
        return err;
    if (m_codec_ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
        m_frame_size_= 10000;
    } else {
        m_frame_size = m_codec_ctx->frame_size;
    }

    m_codec_ctx.extract_parameters(m_stream->codecpar);
    m_swr.set_opts( m_codec_ctx->channel_layout,
                    m_codec_ctx->sample_fmt,
                    m_codec_ctx->sample_rate,
                    av_get_default_channel_layout(2),
                    AV_SAMPLE_FMT_FLT,
                    samplerate);

    m_frame_swr.get_audio_buffer(m_codec_ctx->sample_fmt, m_codec_ctx->channels, m_frame_size);
    m_frame_swr->sample_rate = m_codec_ctx->sample_rate;
    m_frame_orig.get_audio_buffer(AV_SAMPLE_FMT_FLT, 2, int(m_frame_size * ( samplerate * 1. / m_codec_ctx->sample_rate))).
    m_frame_orig->sample_rate = samplerate;
    // qDebug() << "jepusti";
    return 0;
}
