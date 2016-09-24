#ifndef MIXXX_SOUNDSOURCEPROVIDER_H
#define MIXXX_SOUNDSOURCEPROVIDER_H

#include <QString>
#include <QStringList>
#include <QUrl>

#include "sources/soundsource.h"

namespace mixxx {
// Factory interface for SoundSources
//
// The implementation of a SoundSourceProvider must be thread-safe, because
// a single instance might be accessed concurrently from different threads.
class SoundSourceProvider {
public:
    virtual ~SoundSourceProvider() = default;
    // A user-readable name that identifies this provider.
    virtual QString getName() const = 0;
    // A list of supported file extensions in any order.
    virtual QStringList getSupportedFileExtensions() const = 0;
    // Creates a new SoundSource for the file referenced by the URL.
    // This function should return a nullptr pointer if it is already
    // able to decide that the file is not supported even though it
    // has one of the supported file extensions.
    virtual SoundSourcePointer newSoundSource(QUrl url) = 0;
    virtual bool canOpen(QUrl url) = 0;
    virtual bool canOpen(QString url) = 0;
};

typedef QSharedPointer<SoundSourceProvider> SoundSourceProviderPointer;

template<typename T>
static SoundSourceProviderPointer newSoundSourceProvider()
{
    return SoundSourceProviderPointer(new T);
}

} // namespace mixxx

#endif // MIXXX_SOUNDSOURCEPROVIDER_H
