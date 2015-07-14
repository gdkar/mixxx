#ifndef MIXXX_SOUNDSOURCEFFMPEG_H
#define MIXXX_SOUNDSOURCEFFMPEG_H

#include "sources/soundsourceprovider.h"


// Needed to ensure that macros in <stdint.h> get defined.
#ifndef __STDC_CONSTANT_MACROS
#if __cplusplus < 201103L
#define __STDC_CONSTANT_MACROS
#endif
#endif

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavfilter/avfilter.h>
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>

#ifndef __FFMPEGOLDAPI__
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#endif

// Compability
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
};
#include <QVector>

namespace Mixxx {
class SoundSourceFFmpeg : public SoundSource {
public:
    explicit SoundSourceFFmpeg(QUrl url);
    ~SoundSourceFFmpeg();
    void close() override;
    SINT seekSampleFrame(SINT frameIndex) override;
    SINT readSampleFrames(SINT numberOfFrames, CSAMPLE* sampleBuffer) override;
private:
    Result tryOpen(const AudioSourceConfig& audioSrcCfg) override;
    void readFramesToCache(unsigned int count, SINT offset);
    AVFormatContext *m_pFormatCtx;
    int m_iAudioStream;
    AVCodecContext *m_pCodecCtx;
    AVCodec *m_pCodec;
    SwrContext *m_pSwrCtx;
    SINT m_currentMixxxFrameIndex = 0;
    QVector<AVFrame *>  m_frameCache;
    off_t               m_cacheStart;
    off_t               m_cacheEnd;
};

class SoundSourceProviderFFmpeg: public SoundSourceProvider {
public:
    QString getName() const override {return "FFmpeg";}
    QStringList getSupportedFileExtensions() const override;
    SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceFFmpeg(url));
    }
};

} // namespace Mixxx

#endif // MIXXX_SOUNDSOURCEFFMPEG_H
