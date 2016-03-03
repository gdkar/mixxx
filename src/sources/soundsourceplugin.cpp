#include "soundsourceplugin.h"
#include "defs_version.h"


namespace Mixxx {

    namespace {
        void deleteSoundSource(SoundSource* pSoundSource) {
            // The SoundSource must be deleted from within the external library
            // that has allocated it.
            delete pSoundSource;
        }
    } // anonymous namespace
    SoundSourcePointer exportSoundSourcePlugin(SoundSourcePlugin* pSoundSourcePlugin)
    {
        return SoundSourcePointer(pSoundSourcePlugin, deleteSoundSource);
    }
    SoundSourcePlugin::SoundSourcePlugin(const QUrl& url)
            : SoundSource(url)
    {
    }
    SoundSourcePlugin::SoundSourcePlugin(const QUrl& url, const QString& type)
            : SoundSource(url, type)
    {
    }
} // namespace Mixxx
extern "C" MIXXX_SOUNDSOURCEPLUGINAPI_EXPORT const char* Mixxx_getVersion()
{
    return VERSION;
}
extern "C" MIXXX_SOUNDSOURCEPLUGINAPI_EXPORT int Mixxx_SoundSourcePluginAPI_getVersion()
{
    return MIXXX_SOUNDSOURCEPLUGINAPI_VERSION;
}
