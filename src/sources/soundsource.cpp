#include "sources/soundsource.h"
#include "metadata/trackmetadatataglib.h"
namespace Mixxx {
/*static*/ QString SoundSource::getFileExtensionFromUrl(const QUrl& url) {
    return url.toString().section(".", -1).toLower().trimmed();
}
SoundSource::SoundSource(const QUrl& url)
        : AudioSource(url),
          // simply use the file extension as the type
          m_type(getFileExtensionFromUrl(url)) {
    DEBUG_ASSERT(getUrl().isValid());
}
SoundSource::SoundSource(const QUrl& url, const QString& type)
        : AudioSource(url),
          m_type(type) {
    DEBUG_ASSERT(getUrl().isValid());
}
bool SoundSource::open(const AudioSourceConfig& audioSrcCfg) {
    close(); // reopening is not supported
    auto result = false;
    try { result = tryOpen(audioSrcCfg); }
    catch (...) {
        close();
        throw;
    }
    if (!result) { close(); }
    return result;
}
bool SoundSource::parseTrackMetadataAndCoverArt( TrackMetadata* pTrackMetadata, QImage* pCoverArt) const {
    return readTrackMetadataAndCoverArtFromFile(pTrackMetadata, pCoverArt, getLocalFileName());
}
} //namespace Mixxx
