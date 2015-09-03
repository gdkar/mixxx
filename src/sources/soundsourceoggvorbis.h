#ifndef MIXXX_SOUNDSOURCEOGGVORBIS_H
#define MIXXX_SOUNDSOURCEOGGVORBIS_H

#include "sources/soundsourceprovider.h"

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

namespace Mixxx {

class SoundSourceOggVorbis: public SoundSource {
public:
    explicit SoundSourceOggVorbis(QUrl url);
    virtual ~SoundSourceOggVorbis();
    virtual void close() override;
    virtual SINT seekSampleFrame(SINT frameIndex) override;
    virtual SINT readSampleFrames(SINT numberOfFrames, CSAMPLE* sampleBuffer) override;
    virtual SINT readSampleFramesStereo(SINT numberOfFrames,
            CSAMPLE* sampleBuffer, SINT sampleBufferSize) override;
private:
    Result tryOpen(const AudioSourceConfig& audioSrcCfg) override;

    SINT readSampleFrames(SINT numberOfFrames,
            CSAMPLE* sampleBuffer, SINT sampleBufferSize,
            bool readStereoSamples);
    OggVorbis_File m_vf;
    SINT m_curFrameIndex;
};

class SoundSourceProviderOggVorbis: public SoundSourceProvider {
public:
    virtual QString getName() const override;
    virtual QStringList getSupportedFileExtensions() const override;
    virtual SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceOggVorbis(url));
    }
};

} // namespace Mixxx

#endif // MIXXX_SOUNDSOURCEOGGVORBIS_H
