/****************************************************************************
                   encoderffmpegcore.cpp  -  FFMPEG encoder for mixxx
                             -------------------
    copyright            : (C) 2012-2013 by Tuukka Pasanen
                           (C) 2007 by Wesley Stessens
                           (C) 1994 by Xiph.org (encoder example)
                           (C) 1994 Tobias Rafreider (shoutcast and recording fixes)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "encoder/encoderffmpegresample.h"

EncoderFfmpegResample::EncoderFfmpegResample(AVCodecContext *codecCtx) 
  : m_pCodecCtx(codecCtx),
  , m_pSwrCtx(swr_alloc()){
}

EncoderFfmpegResample::~EncoderFfmpegResample() {
    swr_free(&m_pSwrCtx);
}

int EncoderFfmpegResample::open(int64_t inLayout,  enum AVSampleFormat inSampleFmt,  int inRate,
                                int64_t outLayout, enum AVSampleFormat outSampleFmt, int outRate) {
    m_outLayout    = outLayout;
    m_outSampleFmt = outSampleFmt;
    m_outRate      = outRate;
    m_inLayout     = inLayout;
    m_inSampleFmt  = inSampleFmt;
    m_inRate       = inRate;
    // Some MP3/WAV don't tell this so make assumtion that
    // They are stereo not 5.1
    if (m_pCodecCtx->channel_layout == 0 && m_pCodecCtx->channels == 2) {
        m_pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    } else if (m_pCodecCtx->channel_layout == 0 && m_pCodecCtx->channels == 1) {
        m_pCodecCtx->channel_layout = AV_CH_LAYOUT_MONO;
    } else if (m_pCodecCtx->channel_layout == 0 && m_pCodecCtx->channels == 0) {
        m_pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
        m_pCodecCtx->channels = 2;
    }
    m_pSwrCtx = swr_alloc_set_opts(m_pSwrCtx,
        m_outLayout,    // output layout
        m_outSampleFmt, // output format
        m_outRate,      // output rate
        m_inLayout,     // input  layout
        m_inSampleFmt,  // input  format
        m_inRate,       // input  rate
        0,              // log    context offset
        nullptr         // log    context
      );

    // They made a big change in FFPEG 1.1 before that every format just passed
    // s16 back to applications. from FFMPEG 1.1 up MP3 pass s16p (Planar stereo
    // 16 bit) MP4/AAC FLTP (Planar 32 bit float) and OGG also FLTP
    // (WMA same thing) If sample type ain't FLT (packed float) example FFMPEG 1.1
    // mp3 is s16p that ain't and mp4 FLTP (32 bit float)
    // NOT Going to work because MIXXX works with pure s16 that is not planar
    // GOOD thing is now this can handle allmost everything..
    // What should be tested is 48000 Hz downsample and 22100 Hz up sample.

    if (!m_pSwrCtx || (swr_init(m_pSwrCtx)<0)) {
        qDebug() << "Can't init convertor!";
        return -1;
    }
    return 0;
}

int EncoderFfmpegResample::reSample(AVFrame *inframe, AVFrame *outframe) {
    if(!m_pSwrCtx || !inframe || !outframe)
    return AVERROR(EINVAL);
    av_frame_unref(outframe);
    outframe->rate       = m_outRate;
    outframe->format     = m_outSampleFmt;
    outframe->layout     = m_outLayout;
    outframe->channels   = av_get_channel_layout_nb_channels(m_outLayout);
    outframe->nb_samples = 0;
    if(swr_convert_frame(m_pSwrCtx,outframe,inframe)<0 && (
          swr_config_frame(m_pSwrCtx,outframe,inframe)<0 ||
          swr_convert_frame(m_pSwrCtx,outframe,inframe)<0))
      return -1;
    return 0;
}
