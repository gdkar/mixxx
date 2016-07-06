#ifndef MIXXX_SOUNDSOURCEOPUS_H
#define MIXXX_SOUNDSOURCEOPUS_H

#include "sources/soundsourceprovider.h"

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <opus/opusfile.h>

namespace mixxx {

class SoundSourceOpus: public mixxx::SoundSource {
public:
    explicit SoundSourceOpus(const QUrl& url);
    ~SoundSourceOpus() override;

    Result parseTrackMetadataAndCoverArt(
            TrackMetadata* pTrackMetadata,
            QImage* pCoverArt) const override;

    void close() override;

    int64_t seekSampleFrame(int64_t frameIndex) override;

    int64_t readSampleFrames(int64_t numberOfFrames,
            CSAMPLE* sampleBuffer) override;
    int64_t readSampleFramesStereo(int64_t numberOfFrames,
            CSAMPLE* sampleBuffer, int64_t sampleBufferSize) override;

private:
    OpenResult tryOpen(const AudioSourceConfig& audioSrcCfg) override;

    OggOpusFile *m_pOggOpusFile;

    int64_t m_curFrameIndex;
};

class SoundSourceProviderOpus: public SoundSourceProvider {
public:
    QString getName() const override;

    QStringList getSupportedFileExtensions() const override;

    SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceOpus(url));
    }
};

} // namespace mixxx

#endif // MIXXX_SOUNDSOURCEOPUS_H
