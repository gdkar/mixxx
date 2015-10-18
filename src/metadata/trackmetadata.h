_Pragma("once")
#include <QDateTime>
#include <QObject>
#include <cmath>

namespace Mixxx {

class TrackMetadata : public QObject{
    Q_OBJECT
    Q_PROPERTY(QString artist READ getArtist WRITE setArtist NOTIFY updated);
    Q_PROPERTY(QString album READ getAlbum WRITE setAlbum NOTIFY updated);
    Q_PROPERTY(QString title READ getTitle WRITE setTitle NOTIFY updated);
    Q_PROPERTY(QString album_artist READ getAlbumArtist WRITE setAlbumArtist NOTIFY updated);
    Q_PROPERTY(QString genre READ getGenre WRITE setGenre NOTIFY updated);
    Q_PROPERTY(QString comment READ getComment WRITE setComment NOTIFY updated);
    Q_PROPERTY(QString year READ getYear WRITE setYear NOTIFY updated);
    Q_PROPERTY(QString track_number READ getTrackNumber WRITE setTrackNumber NOTIFY updated);
    Q_PROPERTY(QString composer READ getComposer WRITE setComposer NOTIFY updated);
    Q_PROPERTY(QString grouping READ getGrouping WRITE setGrouping NOTIFY updated);
    Q_PROPERTY(QString key READ getKey WRITE setKey NOTIFY updated);
    Q_PROPERTY(int channels READ getChannels WRITE setChannels NOTIFY updated);
    Q_PROPERTY(int sample_rate READ getSampleRate WRITE setSampleRate NOTIFY updated);
    Q_PROPERTY(int bitrate READ getBitrate WRITE setBitrate NOTIFY updated);
    Q_PROPERTY(int duration READ getDuration WRITE setDuration NOTIFY updated);
    Q_PROPERTY(double bpm READ getBpm WRITE setBpm RESET resetBpm NOTIFY updated);
    Q_PROPERTY(double replay_gain READ getReplayGain WRITE setReplayGain RESET resetReplayGain NOTIFY updated);
public:
    TrackMetadata();
    virtual ~TrackMetadata();
    QString getArtist() const;
    void setArtist(QString artist);
    QString getTitle() const;
    void setTitle(QString title);
    QString getAlbum() const;
    void setAlbum(QString album);
    QString getAlbumArtist() const;
    void setAlbumArtist(QString albumArtist);
    QString getGenre() const;
    void setGenre(QString genre);
    QString getComment() const;
    void setComment(QString comment);
    // year, date or date/time formatted according to ISO 8601
    QString getYear() const;
    void setYear(QString year);
    QString getTrackNumber() const;
    void setTrackNumber(QString trackNumber);
    QString getComposer() const;
    void setComposer(QString composer);
    QString getGrouping() const;
    void setGrouping(QString grouping);
    QString getKey() const;
    void setKey(QString key);
    // #channels
    int getChannels() const;
    void setChannels(int channels);
    // Hz
    int getSampleRate() const;
    void setSampleRate(int sampleRate);
    // kbit / s
    int getBitrate() const;
    void setBitrate(int bitrate);
    // #seconds
    int getDuration() const;
    void setDuration(int duration);
    // beats / minute
    static const double kBpmUndefined;
    static const double kBpmMin; // lower bound (exclusive)
    static const double kBpmMax; // upper bound (inclusive)
    double getBpm() const;
    int getBpmAsInteger() const;
    static bool isBpmValid(double bpm);
    bool isBpmValid() const;
    void setBpm(double bpm);
    void resetBpm();
    static const double kReplayGainUndefined;
    static const double kReplayGainMin; // lower bound (exclusive)
    static const double kReplayGain0dB;
    double getReplayGain() const;
    static bool isReplayGainValid(double replayGain);
    bool isReplayGainValid() const;
    void setReplayGain(double replayGain);
    void resetReplayGain();
    // Parse and format BPM metadata
    static double parseBpm(const QString& sBpm, bool* pValid = 0);
    static QString formatBpm(double bpm);
    static QString formatBpm(int bpm);

    // Parse and format replay gain metadata according to the
    // ReplayGain 1.0 specification.
    // http://wiki.hydrogenaud.io/index.php?title=ReplayGain_1.0_specification
    static double parseReplayGain(QString sReplayGain, bool* pValid = 0);
    static QString formatReplayGain(double replayGain);

    // Parse an format date/time values according to ISO 8601
    static QDate     parseDate(QString str);
    static QDateTime parseDateTime(QString str);
    static QString   formatDate(QDate date);
    static QString   formatDateTime(QDateTime dateTime);
    // Parse and format the calendar year (for simplified display)
    static const int kCalendarYearInvalid;
    static int parseCalendarYear(QString year, bool* pValid = 0);
    static QString formatCalendarYear(QString year, bool* pValid = 0);
    static QString reformatYear(QString year);
signals:
    void updated();
private:
    QString m_artist;
    QString m_title;
    QString m_album;
    QString m_albumArtist;
    QString m_genre;
    QString m_comment;
    QString m_year;
    QString m_trackNumber;
    QString m_composer;
    QString m_grouping;
    QString m_key;

    // The following members need to be initialized
    // explicitly in the constructor! Otherwise their
    // value is undefined.
    int m_channels = 2;
    int m_sampleRate = 44100;
    int m_bitrate = 0;
    int m_duration = 0;
    double m_bpm = 0;
    double m_replayGain = 1.0;
};
}
