#ifndef MIXXX_SOURCES_SOUNDSOURCEPROXY_H
#define MIXXX_SOURCES_SOUNDSOURCEPROXY_H

#include <QRegExp>
#include "track/track.h"

#include "sources/soundsourceprovider.h"
#include "sources/soundsource.h"
namespace mixxx {
// Creates sound sources for tracks. Only intended to be used
// in a narrow scope and not sharable between multiple threads!
class SoundSourceProxy {
  public:
    // Initially registers all built-in SoundSource providers and
    // loads all SoundSource plugins with additional providers. This
    // function is not thread-safe and must be called only once
    // upon startup of the application.
    static void loadPlugins();
    static QStringList getSupportedFileExtensions();
    static QStringList getSupportedFileNamePatterns();
    static const QRegExp &getSupportedFileNameRegex();
    static bool isUrlSupported(QUrl url);
    static bool isFileSupported(const QFileInfo& fileInfo);
    static bool isFileNameSupported(const QString& fileName);
    static bool isFileExtensionSupported(const QString& fileExtension);
    explicit SoundSourceProxy(const TrackPointer& pTrack);
    const TrackPointer& getTrack() const { return m_pTrack; }
    QUrl getUrl() const { return m_url; }
    // Load track metadata and (optionally) cover art from the file
    // if it has not already been parsed. With reloadFromFile = true
    // metadata and cover art will be reloaded from the file regardless
    // if it has already been parsed before or not.
    void loadTrackMetadata(bool reloadFromFile = false) const
    {
        return loadTrackMetadataAndCoverArt(false, reloadFromFile);
    }
    void loadTrackMetadataAndCoverArt(bool reloadFromFile = false) const
    {
        return loadTrackMetadataAndCoverArt(true, reloadFromFile);
    }
    // Parse only the metadata from the file without modifying
    // the referenced track.
    Result parseTrackMetadata(TrackMetadata* pTrackMetadata) const;
    // Parse only the cover image from the file without modifying
    // the referenced track.
    QImage parseCoverImage() const;
    enum class SaveTrackMetadataResult {
        SUCCEEDED,
        FAILED,
        SKIPPED
    };
    static SaveTrackMetadataResult saveTrackMetadata(
            const Track* pTrack,
            bool evenIfNeverParsedFromFileBefore = false);
    // Opening the audio data through the proxy will
    // update the some metadata of the track object.
    // Returns a null pointer on failure.
    AudioSourcePointer openAudioSource(
            const AudioSourceConfig& audioSrcCfg = AudioSourceConfig());
    void closeAudioSource();
  private:
    static QList<SoundSourceProviderPointer> s_soundSourceProviders;
    static QStringList                       s_supportedFileNamePatterns;
    static QStringList                       s_supportedFileExtensions;
    static QRegExp                           s_supportedFileNameRegex;
    // Special case: Construction from a plain TIO pointer is needed
    // for writing metadata immediately before the TIO is destroyed.
    explicit SoundSourceProxy(const Track* pTrack);

    const TrackPointer m_pTrack;
    const QUrl m_url;
    SoundSourceProviderPointer getSoundSourceProvider() const;
    void nextSoundSourceProvider();
    void initSoundSource();
    void loadTrackMetadataAndCoverArt(bool withCoverArt, bool reloadFromFile) const;
    int m_soundSourceProviderIndex{0};
    // This pointer must stay in this class together with
    // the corresponding track pointer. Don't pass it around!!
    SoundSourcePointer m_pSoundSource;
    // Keeps track of opening and closing the corresponding
    // SoundSource. This pointer can safely be passed around,
    // because internally it contains a reference to the TIO
            // that keeps it alive.
    AudioSourcePointer m_pAudioSource;
};
}
using mixxx::SoundSourceProxy;
#endif // MIXXX_SOURCES_SOUNDSOURCEPROXY_H
