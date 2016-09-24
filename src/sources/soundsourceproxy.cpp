#include <QApplication>
#include <QDesktopServices>

#include "sources/soundsourceproxy.h"

#if 0 && defined(__MPG123__)
#include "sources/soundsourcemp3.h"
#endif
#include "sources/soundsourceffmpeg.h"

#include "library/coverartutils.h"
#include "library/coverartcache.h"
#include "util/cmdlineargs.h"
#include "util/regex.h"

using namespace mixxx;

//Static memory allocation
/*static*/ QList<SoundSourceProviderPointer> SoundSourceProxy::s_soundSourceProviders;
QStringList SoundSourceProxy::s_supportedFileExtensions{};
QStringList SoundSourceProxy::s_supportedFileNamePatterns{};
QRegExp     SoundSourceProxy::s_supportedFileNameRegex{};
namespace {
QUrl getCanonicalUrlForTrack(const Track* pTrack)
{
    if (pTrack == nullptr)
        // Missing track
        return QUrl();
    auto canonicalLocation = pTrack->getCanonicalLocation();
    if (canonicalLocation.isEmpty()) {
        // Corresponding file is missing or inaccessible
        //
        // NOTE(uklotzde): Special case handling is required for Qt 4.8!
        // Creating an URL from an empty local file in Qt 4.8 will result
        // in an URL with the string "file:" instead of an empty URL.
        //
        // TODO(XXX): This is no longer required for Qt 5.x
        // http://doc.qt.io/qt-5/qurl.html#fromLocalFile
        // "An empty localFile leads to an empty URL (since Qt 5.4)."
        return QUrl();
    }
    return QUrl::fromLocalFile(canonicalLocation);
}
} // anonymous namespace
// static
void SoundSourceProxy::loadPlugins()
{
    // Initialize built-in file types.
    // Fallback providers should be registered before specialized
    // providers to ensure that they are only after the specialized
    // provider failed to open a file. But the order of registration
    // only matters among providers with equal priority.
    // Use FFmpeg as the last resort.
    s_soundSourceProviders.append(
        newSoundSourceProvider<SoundSourceProviderFFmpeg>()
        );
    s_supportedFileExtensions.clear();
    s_supportedFileNamePatterns.clear();
    for(auto & provider : s_soundSourceProviders)
        s_supportedFileExtensions += provider->getSupportedFileExtensions();
    for(auto && extension : s_supportedFileExtensions) {
        s_supportedFileNamePatterns.append(QString("*.%1").arg(extension));
    }
    auto regex_source = RegexUtils::fileExtensionsRegex(s_supportedFileExtensions);
    s_supportedFileNameRegex = QRegExp(regex_source, Qt::CaseInsensitive);

}
// static
bool SoundSourceProxy::isUrlSupported(QUrl url)
{
    for(auto && provider : s_soundSourceProviders) {
        if(provider->canOpen(url))
            return true;
    }
    const QFileInfo fileInfo(url.toLocalFile());
    return isFileSupported(fileInfo);
}
// static
bool SoundSourceProxy::isFileSupported(const QFileInfo& fileInfo)
{
    for(auto && provider : s_soundSourceProviders) {
        if(provider->canOpen(fileInfo.canonicalFilePath()))
            return true;
    }
    return isFileNameSupported(fileInfo.fileName());
}
// static
bool SoundSourceProxy::isFileNameSupported(const QString& fileName)
{
    return fileName.contains(s_supportedFileNameRegex);
}
// static
bool SoundSourceProxy::isFileExtensionSupported(const QString& fileExtension)
{
    return getSupportedFileExtensions().contains(fileExtension);
}
//static
SoundSourceProxy::SaveTrackMetadataResult SoundSourceProxy::saveTrackMetadata(
        const Track* pTrack,
        bool evenIfNeverParsedFromFileBefore)
{
    DEBUG_ASSERT(nullptr != pTrack);
    SoundSourceProxy proxy(pTrack);
    if (proxy.m_pSoundSource) {
        mixxx::TrackMetadata trackMetadata;
        auto parsedFromFile = false;
        pTrack->getTrackMetadata(&trackMetadata, &parsedFromFile);
        if (parsedFromFile || evenIfNeverParsedFromFileBefore) {
            switch (proxy.m_pSoundSource->writeTrackMetadata(trackMetadata)) {
            case OK:
                qDebug() << "Track metadata has been written into file"
                        << pTrack->getLocation();
                return SaveTrackMetadataResult::SUCCEEDED;
            case ERR:
                break;
            default:
                DEBUG_ASSERT(!"unreachable code");
            }
        } else {
            qDebug() << "Skip writing of track metadata into file"
                    << pTrack->getLocation();
            return SaveTrackMetadataResult::SKIPPED;
        }
    }
    qDebug() << "Failed to write track metadata into file"
            << pTrack->getLocation();
    return SaveTrackMetadataResult::FAILED;
}
SoundSourceProxy::SoundSourceProxy(const TrackPointer& pTrack)
    : m_pTrack(pTrack),
      m_url(getCanonicalUrlForTrack(pTrack.data()))
{
    initSoundSource();
}
SoundSourceProxy::SoundSourceProxy(const Track* pTrack)
    : m_url(getCanonicalUrlForTrack(pTrack))
{
    initSoundSource();
}
SoundSourceProviderPointer SoundSourceProxy::getSoundSourceProvider() const
{
    if(m_soundSourceProviderIndex >= 0 && m_soundSourceProviderIndex <
        s_soundSourceProviders.size()) {
        return s_soundSourceProviders[m_soundSourceProviderIndex];
    }else{
        return SoundSourceProviderPointer();
    }
}
void SoundSourceProxy::nextSoundSourceProvider()
{
    if (s_soundSourceProviders.size() > m_soundSourceProviderIndex) {
        ++m_soundSourceProviderIndex;
        // Discard SoundSource and AudioSource from previous provider
        closeAudioSource();
        m_pSoundSource = SoundSourcePointer();
    }
}
void SoundSourceProxy::initSoundSource()
{
    DEBUG_ASSERT(!m_pSoundSource);
    DEBUG_ASSERT(!m_pAudioSource);
    while (!m_pSoundSource) {
        auto pProvider = getSoundSourceProvider();
        if (!pProvider) {
            if (!getUrl().isEmpty()) {
                qWarning() << "No SoundSourceProvider for file" << getUrl().toString();
            }
            // Failure
            return;
        }
        m_pSoundSource = pProvider->newSoundSource(m_url);
        if (!m_pSoundSource) {
            qWarning() << "SoundSourceProvider"
                       << pProvider->getName()
                       << "failed to create a SoundSource for file"
                       << getUrl().toString();
            // Switch to next provider...
            nextSoundSourceProvider();
            // ...and continue loop
            DEBUG_ASSERT(!m_pSoundSource);
        } else {
            auto trackType = m_pSoundSource->getType();
/*            qDebug() << "SoundSourceProvider"
                     << pProvider->getName()
                     << "created a SoundSource for file"
                     << getUrl().toString()
                     << "of type"
                     << trackType;*/
            if (m_pTrack) {
                m_pTrack->setType(trackType);
            }
        }
    }
}
namespace {
    // Parses artist/title from the file name and returns the file type.
    // Assumes that the file name is written like: "artist - title.xxx"
    // or "artist_-_title.xxx".
    // This function does not overwrite any existing (non-empty) artist
    // and title fields!
    void parseMetadataFromFileName(mixxx::TrackMetadata* pTrackMetadata, QString fileName)
    {
        fileName.replace("_", " ");
        auto titleWithFileType = QString{};
        if (fileName.count('-') == 1) {
            if (pTrackMetadata->getArtist().isEmpty()) {
                auto artist = fileName.section('-', 0, 0).trimmed();
                if (!artist.isEmpty()) {
                    pTrackMetadata->setArtist(artist);
                }
            }
            titleWithFileType = fileName.section('-', 1, 1).trimmed();
        } else {
            titleWithFileType = fileName.trimmed();
        }
        if (pTrackMetadata->getTitle().isEmpty()) {
            auto title = titleWithFileType.section('.', 0, -2).trimmed();
            if (!title.isEmpty()) {
                pTrackMetadata->setTitle(title);
            }
        }
    }
} // anonymous namespace

void SoundSourceProxy::loadTrackMetadataAndCoverArt(
        bool withCoverArt,
        bool reloadFromFile) const
{
    DEBUG_ASSERT(m_pTrack);
    if (!m_pSoundSource) {
        // Silently ignore requests for unsupported files
        qDebug() << "Unable to parse file tags without a SoundSource" << getUrl().toString();
        return;
    }
    // Use the existing trackMetadata as default values. Otherwise
    // existing values in the library will be overwritten with
    // empty values if the corresponding file tags are missing.
    // Depending on the file type some kind of tags might even
    // not be supported at all and those would get lost!
    auto parsedFromFile = false;
    auto trackMetadata = TrackMetadata{};
    m_pTrack->getTrackMetadata(&trackMetadata, &parsedFromFile);
    if (parsedFromFile && !reloadFromFile) {
        qDebug() << "Skip parsing of track metadata from file" << getUrl().toString();
        return; // do not reload from file
    }
    // If parsing of the cover art image should be omitted the
    // 2nd output parameter must be set to nullptr. Cover art
    // is not reloaded from file once the metadata has been parsed!
    CoverInfoRelative coverInfoRelative;
    QImage coverImg;
    DEBUG_ASSERT(coverImg.isNull());
    auto pCoverImg = (withCoverArt && !parsedFromFile) ? &coverImg : static_cast<QImage*>(nullptr);
    auto parsedCoverArt = false;
    // Parse the tags stored in the audio file.
    if (m_pSoundSource &&
            (m_pSoundSource->parseTrackMetadataAndCoverArt(&trackMetadata, pCoverImg) == OK)) {
        parsedFromFile = true;
        if (!coverImg.isNull()) {
            // Cover image has been parsed from the file
            // TODO() here we may introduce a duplicate hash code
            coverInfoRelative.hash = CoverArtUtils::calculateHash(coverImg);
            coverInfoRelative.coverLocation = QString();
            coverInfoRelative.type = CoverInfo::METADATA;
            coverInfoRelative.source = CoverInfo::GUESSED;
            parsedCoverArt = true;
        }
    } else {
        qWarning() << "Failed to parse track metadata from file" << getUrl().toString();
        if (parsedFromFile) {
            // Don't overwrite any existing metadata that once has
            // been parsed successfully from file.
            return;
        }
    }
    // If Artist or title fields are blank try to parse them
    // from the file name.
    // TODO(rryan): Should we re-visit this decision?
    if (trackMetadata.getArtist().isEmpty() || trackMetadata.getTitle().isEmpty()) {
        parseMetadataFromFileName(&trackMetadata, m_pTrack->getFileInfo().fileName());
    }
    // Dump the trackMetadata extracted from the file back into the track.
    m_pTrack->setTrackMetadata(trackMetadata, parsedFromFile);
    if (parsedCoverArt) {
        m_pTrack->setCoverInfo(coverInfoRelative);
    }
}
Result SoundSourceProxy::parseTrackMetadata(mixxx::TrackMetadata* pTrackMetadata) const
{
    if (m_pSoundSource) {
        return m_pSoundSource->parseTrackMetadataAndCoverArt(pTrackMetadata, nullptr);
    } else {
        return ERR;
    }
}
QImage SoundSourceProxy::parseCoverImage() const
{
    QImage coverImg;
    if (m_pSoundSource) {
        m_pSoundSource->parseTrackMetadataAndCoverArt(nullptr, &coverImg);
    }
    return coverImg;
}
namespace {

// Keeps the TIO alive while accessing the audio data
// of the track. The TIO must not be deleted while
// accessing the corresponding file to avoid file
// corruption when writing metadata while the file
// is still in use.
class AudioSourceProxy: public mixxx::AudioSource {
public:
    AudioSourceProxy(const AudioSourceProxy&) = delete;
    AudioSourceProxy(AudioSourceProxy&&) = delete;
    static AudioSourcePointer create(
            TrackPointer pTrack,
            AudioSourcePointer pAudioSource)
    {
        DEBUG_ASSERT(pTrack);
        DEBUG_ASSERT(pAudioSource);
        return mixxx::AudioSourcePointer(new AudioSourceProxy(pTrack, pAudioSource));
    }
    SINT seekSampleFrame(SINT frameIndex) override
    {
        return m_pAudioSource->seekSampleFrame(frameIndex);
    }
    SINT readSampleFrames(SINT numberOfFrames,CSAMPLE* sampleBuffer) override
    {
        return m_pAudioSource->readSampleFrames(numberOfFrames,sampleBuffer);
    }
    SINT readSampleFramesStereo(SINT numberOfFrames,CSAMPLE* sampleBuffer,SINT sampleBufferSize) override
    {
        return m_pAudioSource->readSampleFramesStereo(
                numberOfFrames,
                sampleBuffer,
                sampleBufferSize);
    }
private:
    AudioSourceProxy(
            TrackPointer pTrack,
            mixxx::AudioSourcePointer pAudioSource)
        : AudioSource(*pAudioSource),
          m_pTrack(pTrack),
          m_pAudioSource(pAudioSource)
    { }
    const TrackPointer m_pTrack;
    const mixxx::AudioSourcePointer m_pAudioSource;
};
} // anonymous namespace
mixxx::AudioSourcePointer SoundSourceProxy::openAudioSource(const mixxx::AudioSourceConfig& audioSrcCfg)
{
    DEBUG_ASSERT(m_pTrack);
    while (!m_pAudioSource) {
        if (!m_pSoundSource) {
            qWarning() << "Failed to open AudioSource for file" << getUrl().toString();
            return m_pAudioSource; // failure -> exit loop
        }
        auto openResult = m_pSoundSource->open(audioSrcCfg);
        if (mixxx::SoundSource::OpenResult::UNSUPPORTED_FORMAT != openResult) {
            qDebug() << "Opened AudioSource for file"
                     << getUrl().toString()
                     << "with provider"
                     << getSoundSourceProvider()->getName();
            if ((mixxx::SoundSource::OpenResult::SUCCEEDED == openResult)
              && m_pSoundSource->verifyReadable()) {

                m_pAudioSource = AudioSourceProxy::create(m_pTrack, m_pSoundSource);
                if (m_pAudioSource->isEmpty()) {
                    qWarning() << "Empty audio data in file" << getUrl().toString();
                }
                // Overwrite metadata with actual audio properties
                if (m_pTrack) {
                    m_pTrack->setChannels(m_pAudioSource->getChannelCount());
                    m_pTrack->setSampleRate(m_pAudioSource->getSampleRate());
                    if (m_pAudioSource->hasDuration()) {
                        m_pTrack->setDuration(m_pAudioSource->getDuration());
                    }
                    if (m_pAudioSource->hasBitrate()) {
                        m_pTrack->setBitrate(m_pAudioSource->getBitrate());
                    }
                }
                return m_pAudioSource; // success -> exit loop
            } else {
                qWarning() << "Invalid audio data in file" << getUrl().toString();
                // Do NOT retry with the next SoundSource provider if
                // the file itself is malformed!
                m_pSoundSource->close();
                break; // exit loop
            }
        }
        qWarning() << "Failed to open AudioSource for file"
                   << getUrl().toString()
                   << "with provider"
                   << getSoundSourceProvider()->getName();
        // Continue with the next SoundSource provider
        nextSoundSourceProvider();
        initSoundSource();
    }
    // m_pSoundSource might be invalid when reaching this point
    qWarning() << "Failed to open AudioSource for file" << getUrl().toString();
    return m_pAudioSource;
}
void SoundSourceProxy::closeAudioSource()
{
    if (m_pAudioSource) {
        DEBUG_ASSERT(m_pSoundSource);
        m_pSoundSource->close();
        m_pAudioSource = mixxx::AudioSourcePointer();
        qDebug() << "Closed AudioSource for file" << getUrl().toString();
    }
}
QStringList SoundSourceProxy::getSupportedFileExtensions()
{
    return s_supportedFileExtensions;
}
QStringList SoundSourceProxy::getSupportedFileNamePatterns()
{
    return s_supportedFileNamePatterns;
}
const QRegExp &SoundSourceProxy::getSupportedFileNameRegex()
{
    return s_supportedFileNameRegex;
}
