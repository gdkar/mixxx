#include "sources/soundsource.h"

namespace mixxx {

/*static*/ QString SoundSource::getFileExtensionFromUrl(QUrl url) {
    return url.toString().section(".", -1).toLower().trimmed();
}
bool SoundSource::isLocalFile() const { return getUrl().isLocalFile();}
QString SoundSource::getLocalFileName() const { return getUrl().toLocalFile();}
QUrl SoundSource::getUrl() const { return m_url;}
QString SoundSource::getUrlString() const { return m_url.toString();}
const QString& SoundSource::getType() const { return m_type; }

SoundSource::SoundSource(QUrl url)
        : AudioSource(),
          m_url(url),
          // simply use the file extension as the type
          m_type(getFileExtensionFromUrl(url))
{
    DEBUG_ASSERT(getUrl().isValid());
}

SoundSource::SoundSource(QUrl url, const QString& type)
        : AudioSource(),
          m_url(url),
          m_type(type)
{
    DEBUG_ASSERT(getUrl().isValid());
}

SoundSource::OpenResult SoundSource::open(const AudioSourceConfig& audioSrcCfg)
{
    close(); // reopening is not supported
    OpenResult result;
    try {
        result = tryOpen(audioSrcCfg);
    } catch (const std::exception& e) {
        qWarning() << "Caught unexpected exception from SoundSource::tryOpen():" << e.what();
        result = OpenResult::FAILED;
    } catch (...) {
        qWarning() << "Caught unknown exception from SoundSource::tryOpen()";
        result = OpenResult::FAILED;
    }
    if (OpenResult::SUCCEEDED != result) {
        close(); // rollback
    }
    return result;
}
Result SoundSource::writeTrackMetadata(
        const TrackMetadata& trackMetadata) const
{
    return ERR;
}
} //namespace mixxx
