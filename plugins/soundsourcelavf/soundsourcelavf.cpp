#include "soundsourcelavf.h"

#include "sampleutil.h"

#ifdef __WINDOWS__
#include <io.h>
#include <fcntl.h>
#endif


namespace Mixxx {


QList<QString> SoundSourceLavf::supportedFileExtensions() {
    QList<QString> list;
    list.push_back("mp3");
    list.push_back("mp2");
    list.push_back("mpeg");
    return list;
}

SoundSourceLavf::SoundSourceLavf(QUrl url)
        : SoundSourcePlugin(url, "lavf"),
          m_h(0),
          m_curFrameIndex(kFrameIndexMin) {

}

SoundSourceLavf::~SoundSourceLavf() {
}

Result SoundSourceLavf::tryOpen(SINT channelCountHint) {
    return OK;
}

void SoundSourceLavf::close() {
}

SINT SoundSourceLavf::seekSampleFrame(SINT frameIndex) {
    return -1;
}

SINT SoundSourceLavf::readSampleFrames(
        SINT numberOfFrames, CSAMPLE* sampleBuffer) {
    return -1;
}

} // namespace Mixxx

extern "C" MY_EXPORT const char* getMixxxVersion() {
    return VERSION;
}

extern "C" MY_EXPORT int getSoundSourceAPIVersion() {
    return MIXXX_SOUNDSOURCE_API_VERSION;
}

extern "C" MY_EXPORT Mixxx::SoundSource* getSoundSource(QString fileName) {
    return new Mixxx::SoundSourceLavf(fileName);
}

extern "C" MY_EXPORT char** supportedFileExtensions() {
    const QList<QString> supportedFileExtensions(
            Mixxx::SoundSourceLavf::supportedFileExtensions());
    return Mixxx::SoundSourcePlugin::allocFileExtensions(
            supportedFileExtensions);
}

extern "C" MY_EXPORT void freeFileExtensions(char** fileExtensions) {
    Mixxx::SoundSourcePlugin::freeFileExtensions(fileExtensions);
}
