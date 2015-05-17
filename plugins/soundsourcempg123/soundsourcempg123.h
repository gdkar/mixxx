#ifndef MIXXX_SOUNDSOURCEMPG123_H
#define MIXXX_SOUNDSOURCEMPG123_H

#include "sources/soundsourceplugin.h"

#include "singularsamplebuffer.h"
#include <mpg123.h>

#include <vector>

//As per QLibrary docs: http://doc.trolltech.com/4.6/qlibrary.html#resolve
#ifdef Q_OS_WIN
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif


class SoundSourceMPG123: public Mixxx::SoundSourcePlugin {
public:
    explicit SoundSourceMPG123(QUrl url);
    virtual ~SoundSourceMPG123();

    void close() /*override*/;

    SINT seekSampleFrame(SINT frameIndex) /*override*/;

    SINT readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer) /*override*/;

private:
    Result tryOpen(const AudioSourceConfig& config) /*override*/;

    mpg123_handle *m_h;

    SINT m_curFrameIndex;
};

class SoundSourceProviderMPG123: public Mixxx::SoundSourceProvider{
  public:
    QString getName() const;
    QStringList getSupportedFileTypes() const;
    Mixxx::SoundSourcePointer newSoundSource(const QUrl &url){
      return Mixxx::SoundSourcePointer(new SoundSourceMPG123(url));
    }

};
extern "C" MIXXXX_SOUNDSOURCEPLUGINAPI_EXPORT
Mixxx::SoundSourceProviderPointer Mixxx_SoundSourcePluginAPI_getSoundSourceProvider();

#endif // MIXXX_SOUNDSOURCEMPG123_H
