#ifndef MIXXX_SOUNDSOURCEFFMPEG_H
#define MIXXX_SOUNDSOURCEFFMPEG_H

#include "sources/soundsourceprovider.h"

#include <encoder/encoderffmpegresample.h>

// Needed to ensure that macros in <stdint.h> get defined.
#ifndef __STDC_CONSTANT_MACROS
#if __cplusplus < 201103L
#define __STDC_CONSTANT_MACROS
#endif
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifndef __FFMPEGOLDAPI__
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#endif

// Compability
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>

#include <QVector>

namespace Mixxx {

class SoundSourceFFmpeg : public SoundSource {
public:
    explicit SoundSourceFFmpeg(QUrl url);
    virtual ~SoundSourceFFmpeg();
    virtual void close() override;
    virtual SINT seekSampleFrame(SINT frameIndex) override;
    virtual SINT readSampleFrames(SINT numberOfFrames, CSAMPLE* sampleBuffer) override;
private:
    virtual Result tryOpen(const AudioSourceConfig& audioSrcCfg) override;
    bool readFramesToCache(SINT offset);
    void clearCache();
    unsigned int read(unsigned long size, SAMPLE*);

    AVFormatContext *m_pFormatCtx = nullptr;
    int m_iAudioStream            = -1;
    AVCodecContext  *m_pCodecCtx  = nullptr;
    AVCodec         *m_pCodec     = nullptr;

    EncoderFfmpegResample *m_pResample = nullptr;

    SINT m_currentMixxxFrameIndex      = 0;
    bool m_bIsSeeked                   = false;
    SINT m_lCacheStartFrame            =  0;
    SINT m_lCacheEndFrame              =  0;
    QVector<AVFrame *> m_SCache;
};

class SoundSourceProviderFFmpeg: public SoundSourceProvider {
public:
    virtual QString getName() const override {return "FFmpeg";}
    virtual QStringList getSupportedFileExtensions() const override;
    virtual SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceFFmpeg(url));
    }
};
} // namespace Mixxx

#endif // MIXXX_SOUNDSOURCEFFMPEG_H
