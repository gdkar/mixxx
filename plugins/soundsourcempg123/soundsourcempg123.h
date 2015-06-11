#ifndef MIXXX_SOUNDSOURCEMPG123_H
#define MIXXX_SOUNDSOURCEMPG123_H

#include "sources/soundsourceprovider.h"
#include "sources/soundsourceplugin.h"

#include <mpg123.h>

#include <QFile>
#include <QByteArray>
#include <QString>
#include <QStringList>

namespace Mixxx {

class SoundSourceMpg123: public SoundSourcePlugin {
public:
    explicit SoundSourceMpg123(const QUrl &url);
    virtual ~SoundSourceMpg123();
    virtual void close() override;
    SINT seekSampleFrame(SINT frameIndex) override;
    SINT readSampleFrames(SINT numberOfFrames,
            CSAMPLE* sampleBuffer) override;
private:
    Result tryOpen(const AudioSourceConfig& audioSrcCfg) override;
    static ssize_t io_read ( void *vptr, void *buf, size_t size   );
    static off_t   io_lseek( void *vptr, off_t offset, int whence );
    static void    io_cleanup ( void * vptr );
    off_t          m_fileTell;
    QByteArray     m_fileData;
    // mpg123 decoder
    mpg123_handle     *m_h;
    long               m_rate;
    int                m_nch;
    int                m_enc;
};

class SoundSourceProviderMpg123: public SoundSourceProvider {
public:
    QString getName() const override;
    QStringList getSupportedFileExtensions() const override;
    SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceMpg123(url));
    }
    static void deleter(void *ptr){}
};
} // namespace Mixxx
extern "C" MIXXX_SOUNDSOURCEPLUGINAPI_EXPORT
Mixxx::SoundSourceProviderPointer Mixxx_SoundSourcePluginAPI_geSoundSourceProvider();
#endif // MIXXX_SOUNDSOURCEMP3_H
