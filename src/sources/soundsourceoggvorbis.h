#ifndef MIXXX_SOUNDSOURCEOGGVORBIS_H
#define MIXXX_SOUNDSOURCEOGGVORBIS_H

#include "sources/soundsourceprovider.h"
#include "util/memory.h"

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

class QFile;

namespace mixxx {

class SoundSourceOggVorbis: public SoundSource {
public:
    explicit SoundSourceOggVorbis(const QUrl& url);
    ~SoundSourceOggVorbis() override;

    void close() override;

    int64_t seekSampleFrame(int64_t frameIndex) override;

    int64_t readSampleFrames(int64_t numberOfFrames,
            CSAMPLE* sampleBuffer) override;
    int64_t readSampleFramesStereo(int64_t numberOfFrames,
            CSAMPLE* sampleBuffer, int64_t sampleBufferSize) override;

private:
    OpenResult tryOpen(const AudioSourceConfig& audioSrcCfg) override;

    int64_t readSampleFrames(int64_t numberOfFrames,
            CSAMPLE* sampleBuffer, int64_t sampleBufferSize,
            bool readStereoSamples);

    static size_t ReadCallback(void *ptr, size_t size, size_t nmemb,
            void *datasource);
    static int SeekCallback(void *datasource, ogg_int64_t offset, int whence);
    static int CloseCallback(void *datasource);
    static long TellCallback(void *datasource);
    static ov_callbacks s_callbacks;

    std::unique_ptr<QFile> m_pFile;

    OggVorbis_File m_vf;

    int64_t m_curFrameIndex;
};

class SoundSourceProviderOggVorbis: public SoundSourceProvider {
public:
    QString getName() const override;

    QStringList getSupportedFileExtensions() const override;

    SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceOggVorbis(url));
    }
};

} // namespace mixxx

#endif // MIXXX_SOUNDSOURCEOGGVORBIS_H
