#include "soundsourceproviderregistry.h"

#include "util/regex.h"
namespace Mixxx {
SoundSourceProviderPointer SoundSourceProviderRegistry::registerProvider(
        const SoundSourceProviderPointer& pProvider) {
    Entry entry;
    entry.pProvider = pProvider;
    return registerEntry(entry);
}
SoundSourceProviderPointer SoundSourceProviderRegistry::registerPluginLibrary(
        const SoundSourcePluginLibraryPointer& pPluginLibrary) {
    auto entry = Entry{};
    entry.pProvider = pPluginLibrary->createSoundSourceProvider();
    entry.pPluginLibrary = pPluginLibrary;
    return registerEntry(entry);
}
SoundSourceProviderPointer SoundSourceProviderRegistry::registerEntry(const Entry& entry) {
    DEBUG_ASSERT(m_supportedFileNameRegex.isEmpty());
    DEBUG_ASSERT(entry.pProvider);
    const auto  supportedFileExtensions =  entry.pProvider->getSupportedFileExtensions();
    DEBUG_ASSERT(entry.pPluginLibrary || !supportedFileExtensions.isEmpty());
    if (entry.pPluginLibrary && supportedFileExtensions.isEmpty()) {
        qWarning() << "SoundSource plugin does not support any file types"
                << entry.pPluginLibrary->getFilePath();
    }
    for(auto & supportedFileExtension: supportedFileExtensions) { m_entries.insert(supportedFileExtension, entry); }
    return entry.pProvider;
}
void SoundSourceProviderRegistry::finishRegistration() {
    const auto supportedFileExtensions = getSupportedFileExtensions();
    const auto fileExtensionsRegex =  RegexUtils::fileExtensionsRegex(supportedFileExtensions);
    QRegExp(fileExtensionsRegex, Qt::CaseInsensitive).swap( m_supportedFileNameRegex);
}
QStringList SoundSourceProviderRegistry::getSupportedFileNamePatterns() const {
    const auto supportedFileExtensions = getSupportedFileExtensions();
    // Turn the list into a "*.mp3 *.wav *.etc" style string
    auto supportedFileNamePatterns = QStringList{};
    for(auto & supportedFileExtension: supportedFileExtensions) { supportedFileNamePatterns += QString("*.%1").arg(supportedFileExtension); }
    return supportedFileNamePatterns;
}
} // Mixxx
