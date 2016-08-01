#ifndef ENCODERFFMPEGRESAMPLE_H
#define ENCODERFFMPEGRESAMPLE_H

#include <QtDebug>
#include "util/ffmpeg-utils.h"
class EncoderFfmpegResample {
  public:
    explicit EncoderFfmpegResample(AVCodecContext *codecCtx);
    ~EncoderFfmpegResample();
    int openMixxx(AVSampleFormat inSampleFmt, AVSampleFormat outSampleFmt);

    unsigned int reSampleMixxx(AVFrame *inframe, quint8 **outbuffer);

  private:
    AVCodecContext *m_pCodecCtx;
    enum AVSampleFormat m_pOutSampleFmt;
    enum AVSampleFormat m_pInSampleFmt;

};

#endif
