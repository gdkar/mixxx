#ifndef MIXXX_SOUNDSOURCEOPUS_H
#define MIXXX_SOUNDSOURCEOPUS_H

#include "sources/soundsourceprovider.h"

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <opus/opusfile.h>

namespace Mixxx {

class SoundSourceOpus: public Mixxx::SoundSource {
public:
    static const SINT kFrameRate;
    explicit SoundSourceOpus(QUrl url);
    virtual ~SoundSourceOpus();
    virtual Result parseTrackMetadataAndCoverArt( TrackMetadata* pTrackMetadata, QImage* pCoverArt) const override;
    virtual void close() override;
    virtual SINT seekSampleFrame(SINT frameIndex) override;
    virtual SINT readSampleFrames(SINT numberOfFrames, CSAMPLE* sampleBuffer) override;
    virtual SINT readSampleFramesStereo(SINT numberOfFrames, CSAMPLE* sampleBuffer, SINT sampleBufferSize) override;
private:
    Result tryOpen(const AudioSourceConfig& audioSrcCfg) override;
    OggOpusFile *m_pOggOpusFile;
    SINT m_curFrameIndex;
};
class SoundSourceProviderOpus: public SoundSourceProvider {
public:
    virtual QString getName() const override;
    virtual QStringList getSupportedFileExtensions() const override;
    virtual SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceOpus(url));
    }
};

} // namespace Mixxx

#endif // MIXXX_SOUNDSOURCEOPUS_H
