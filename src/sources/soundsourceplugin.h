_Pragma("once")
#include "sources/soundsourcepluginapi.h"

namespace Mixxx {

// Common base class for SoundSource plugins
class SoundSourcePlugin: public SoundSource {
protected:
    explicit SoundSourcePlugin(const QUrl& url);
    SoundSourcePlugin(const QUrl& url, const QString& type);
};

// Wraps the SoundSourcePlugin allocated with operator new
// into a SoundSourcePointer that ensures that the managed
// object will deleted from within the external library (DLL)
// eventually.
SoundSourcePointer exportSoundSourcePlugin(SoundSourcePlugin* pSoundSourcePlugin);

} // namespace Mixxx

extern "C" MIXXX_SOUNDSOURCEPLUGINAPI_EXPORT const char* Mixxx_getVersion();
extern "C" MIXXX_SOUNDSOURCEPLUGINAPI_EXPORT int Mixxx_SoundSourcePluginAPI_getVersion();
