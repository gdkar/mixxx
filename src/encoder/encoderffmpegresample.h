/* -*- mode:C++; indent-tabs-mode:t; tab-width:8; c-basic-offset:4; -*- */
/***************************************************************************
                          soundsourceffmpeg.h  -  ffmpeg decoder
                             -------------------
    copyright            : (C) 2003 by Cedric GESTES
    email                : goctaf@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ENCODERFFMPEGRESAMPLE_H
#define ENCODERFFMPEGRESAMPLE_H

#include <QtDebug>

extern "C" {
// Needed to ensure that macros in <stdint.h> get defined.
#ifndef __STDC_CONSTANT_MACROS
#if __cplusplus < 201103L
#define __STDC_CONSTANT_MACROS
#endif
#endif

#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavformat/avformat.h>
#include <errno.h>

// libswresample is strictly better than libavresample
// honstly i'm only up for fixing one of these at the moment, so it's
// gonna be the one that works better, is more up to date, has better
// support, and isn't produced by a bunch of jackasses.

#include <libswresample/swresample.h>

#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>

}

class EncoderFfmpegResample {
public:
    EncoderFfmpegResample(AVCodecContext *codecCtx);
    ~EncoderFfmpegResample();
    int open(
        int64_t inLayout
        enum AVSampleFormat inSampleFmt, 
        int inRate,
        int64_t outLayout,
        enum AVSampleFormat outSampleFmt,
        int outRate);
    int reSample(AVFrame *inframe, AVFrame *outframe);
private:
    AVCodecContext *m_pCodecCtx             = nullptr;
    int64_t             m_inLayout          = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat m_inSampleFmt       = AV_SAMPLE_FMT_S16;
    int                 m_inRate            = 44100;
    int64_t             m_outLayout         = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat m_outSampleFmt      = AV_SAMPLE_FMT_FLT;
    int                 m_outRate           = 44100;

// Please choose to use libswresample
// see comment above
    SwrContext         *m_pSwrCtx           = nullptr;


};

#endif
