_Pragma("once")
#include "trackinfoobject.h"
#include "sources/soundsourceffmpeg.h"
#include "util/sandbox.h"

// Creates sound sources for filenames or tracks
class SoundSourceProxy{
public:
    static QStringList getSupportedFileExtensions();
    static QStringList getSupportedFileNamePatterns();
    static QRegExp getSupportedFileNameRegex();

    static bool isUrlSupported(QUrl url);
    static bool isFileSupported(QFileInfo fileInfo);
    static bool isFileNameSupported(QString fileName);
    static bool isFileExtensionSupported(QString fileExtension);
    explicit SoundSourceProxy(QString qFilename, SecurityTokenPointer pToken = SecurityTokenPointer());
    explicit SoundSourceProxy(TrackPointer pTrack);
    QString getType() const;
    bool parseTrackMetadataAndCoverArt( Mixxx::TrackMetadata* pTrackMetadata, QImage* pCoverArt) const;
    // Only for  backward compatibility.
    // Should be removed when no longer needed.
    bool parseTrackMetadata(Mixxx::TrackMetadata* pTrackMetadata);
    // Only for  backward compatibility.
    // Should be removed when no longer needed.
    QImage parseCoverArt() const;
    // Opening the audio data through the proxy will
    // update the some metadata of the track object.
    // Returns a null pointer on failure.
    Mixxx::SoundSourcePointer openSoundSource(Mixxx::SoundSourceConfig audioSrcCfg = Mixxx::SoundSourceConfig());
    void closeSoundSource();
private:
    static Mixxx::SoundSourceProviderPointer s_soundSourceProvider;
    static Mixxx::SoundSourcePointer initialize(QString qFilename);
    const TrackPointer m_pTrack;
    const SecurityTokenPointer m_pSecurityToken;
    Mixxx::SoundSourcePointer m_pSoundSource;
};

