#ifndef MIXXX_SOUNDSOURCEMP3_H
#define MIXXX_SOUNDSOURCEMP3_H

#include "sources/soundsourceprovider.h"
extern "C" {
#   include <mpg123.h>
}
namespace Mixxx {

class SoundSourceMp3: public SoundSource {
public:
    explicit SoundSourceMp3(const QUrl& url);
    ~SoundSourceMp3() override;

    void close() override;

    SINT seekSampleFrame(SINT frameIndex) override;
    SINT readSampleFrames(
            SINT numberOfFrames,
            CSAMPLE* sampleBuffer) override;
private:
    OpenResult tryOpen(const AudioSourceConfig& audioSrcCfg) override;
    mpg123_handle *m_h{nullptr};
};

class SoundSourceProviderMp3: public SoundSourceProvider {
public:
    QString getName() const override;
    QStringList getSupportedFileExtensions() const override;
    SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceMp3(url));
    }
};

} // namespace mixxx

#endif // MIXXX_SOUNDSOURCEMP3_H
