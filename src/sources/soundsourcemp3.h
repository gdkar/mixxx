_Pragma("once")
#include "sources/soundsourceprovider.h"
#include <mpg123.h>

#include <QFile>
#include <QByteArray>
#include <QString>
#include <QStringList>

namespace Mixxx {

class SoundSourceMp3: public SoundSource {
    static ssize_t     io_read ( void *vptr, void *buf, size_t size   );
    static off_t       io_lseek( void *vptr, off_t offset, int whence );
    static void        io_cleanup ( void * vptr );
    Result             tryOpen(const AudioSourceConfig& audioSrcCfg) override;
    off_t              m_fileTell;
    QByteArray         m_fileData;
    // mpg123 decoder
    mpg123_handle     *m_h;
    long               m_rate;
    int                m_nch;
    int                m_enc;
public:
    explicit SoundSourceMp3(const QUrl &url);
    virtual ~SoundSourceMp3();
    virtual void close() override;
    SINT seekSampleFrame(SINT frameIndex) override;
    SINT readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer) ;
    SINT readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer,SINT sampleBufferSize, bool stereo);
    SINT readSampleFramesStereo(SINT numberOfFrames,CSAMPLE*sampleBuffer,SINT sampleBufferSize) override;
};
class SoundSourceProviderMp3: public SoundSourceProvider {
public:
    QString getName() const override;
    QStringList getSupportedFileExtensions() const override;
    SoundSourcePointer newSoundSource(const QUrl& url) override {
        return SoundSourcePointer(new SoundSourceMp3(url));
    }
};
} // namespace Mixxx

