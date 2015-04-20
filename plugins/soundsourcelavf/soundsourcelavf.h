#ifndef MIXXX_SOUNDSOURCELAVF_H
#define MIXXX_SOUNDSOURCELAVF_H

#include "sources/soundsourceplugin.h"

#include "singularsamplebuffer.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libswresample/swresample.h>


#include <vector>

//As per QLibrary docs: http://doc.trolltech.com/4.6/qlibrary.html#resolve
#ifdef Q_OS_WIN
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif

namespace Mixxx {

class SoundSourceLavf: public SoundSourcePlugin {
public:
    static QList<QString> supportedFileExtensions();

    explicit SoundSourceLavf(QUrl url);
    ~SoundSourceLavf();

    void close() /*override*/;

    SINT seekSampleFrame(SINT frameIndex) /*override*/;

    SINT readSampleFrames(SINT numberOfFrames,
            CSAMPLE* sampleBuffer) /*override*/;

private:
    Result tryOpen(SINT channelCountHint) /*override*/;

    mpg123_handle *m_h;

    SINT m_curFrameIndex;
};

} // namespace Mixxx

extern "C" MY_EXPORT const char* getMixxxVersion();
extern "C" MY_EXPORT int getSoundSourceAPIVersion();
extern "C" MY_EXPORT Mixxx::SoundSource* getSoundSource(QString fileName);
extern "C" MY_EXPORT char** supportedFileExtensions();
extern "C" MY_EXPORT void freeFileExtensions(char** fileExtensions);

#endif // MIXXX_SOUNDSOURCELAVF_H
