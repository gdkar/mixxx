/***************************************************************************
                          trackinfoobject.h  -  description
                             -------------------
    begin                : 10 02 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TRACKINFOOBJECT_H
#define TRACKINFOOBJECT_H

#include <atomic>
#include <QDateTime>
#include <QDomNode>
#include <QFileInfo>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QWeakPointer>

#include "library/dao/cue.h"
#include "library/coverart.h"
#include "proto/keys.pb.h"
#include "track/beats.h"
#include "track/keys.h"
#include "util/sandbox.h"
#include "waveform/waveform.h"

class Cue;

class TrackInfoObject;
typedef QSharedPointer<TrackInfoObject> TrackPointer;
typedef QWeakPointer<TrackInfoObject> TrackWeakPointer;

namespace Mixxx {
    class TrackMetadata;
}

class TrackInfoObject : public QObject {
    Q_OBJECT
  public:
    // Initialize a new track with the filename.
    TrackInfoObject(const QString& file="",
                    SecurityTokenPointer pToken=SecurityTokenPointer(),
                    bool parseHeader=true,
                    bool parseCoverArt=false);
    // Initialize track with a QFileInfo class
    TrackInfoObject(const QFileInfo& fileInfo,
                    SecurityTokenPointer pToken=SecurityTokenPointer(),
                    bool parseHeader=true,
                    bool parseCoverArt=false);
    // Creates a new track given information from the xml file.
    TrackInfoObject(const QDomNode &);
    virtual ~TrackInfoObject();
    // Parse file metadata. If no file metadata is present, attempts to extract
    // artist and title information from the filename.
    void parse(bool parseCoverArt);
    // Returns the duration in seconds
    float getDuration() const;
    // Set duration in seconds
    void setDuration(float);
    // Returns the duration as a string: H:MM:SS
    QString getDurationStr() const;
    // Accessors for various stats of the file on disk. These are auto-populated
    // when the TIO is constructed, or when setLocation() is called.
    Q_PROPERTY(QString artist READ getArtist WRITE setArtist NOTIFY changed)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle NOTIFY changed)
    Q_PROPERTY(QString album READ getAlbum WRITE setAlbum NOTIFY changed)
    Q_PROPERTY(QString albumArtist READ getAlbumArtist WRITE setAlbumArtist NOTIFY changed)
    Q_PROPERTY(QString genre READ getGenre WRITE setGenre NOTIFY changed)
    Q_PROPERTY(QString composer READ getComposer WRITE setComposer NOTIFY changed)
    Q_PROPERTY(QString grouping READ getGrouping WRITE setGrouping NOTIFY changed)
    Q_PROPERTY(QString year READ getYear WRITE setYear NOTIFY changed)
    Q_PROPERTY(QString track_number READ getTrackNumber WRITE setTrackNumber NOTIFY changed)
    Q_PROPERTY(int times_played READ getTimesPlayed NOTIFY changed)
    Q_PROPERTY(QString comment READ getComment WRITE setComment NOTIFY changed)
    Q_PROPERTY(double bpm READ getBpm WRITE setBpm NOTIFY changed)
    Q_PROPERTY(QString bpmFormatted READ getBpmStr STORED false)
    Q_PROPERTY(QString key READ getKeyText WRITE setKeyText NOTIFY changed)
    Q_PROPERTY(float duration READ getDuration WRITE setDuration NOTIFY changed)
    Q_PROPERTY(QString durationFormatted READ getDurationStr STORED false)
    // Returns absolute path to the file, including the filename.
    QString getLocation() const;
    QString getCanonicalLocation() const;
    QFileInfo getFileInfo() const;
    SecurityTokenPointer getSecurityToken();
    // Returns the absolute path to the directory containing the file
    QString getDirectory() const;
    // Returns the filename of the file.
    QString getFilename() const;
    // Returns the length of the file in bytes
    int getLength() const;
    // Returns whether the file exists on disk or not. Updated as of the time
    // the TrackInfoObject is created, or when setLocation() is called.
    bool exists() const;
    // Returns ReplayGain
    float getReplayGain() const;
    // Set ReplayGain
    void setReplayGain(float);
    // Returns BPM
    double getBpm() const;
    // Set BPM
    void setBpm(double);
    // Returns BPM as a string
    QString getBpmStr() const;
    // A track with a locked BPM will not be re-analyzed by the beats or bpm
    // analyzer.
    void setBpmLock(bool hasLock);
    bool hasBpmLock() const;
    bool getHeaderParsed() const;
    void setHeaderParsed(bool parsed = true);
    // Returns the user comment
    QString getComment() const;
    // Sets the user commnet
    void setComment(const QString&);
    // Returns the file type
    QString getType() const;
    // Sets the type of the string
    void setType(const QString&);
    // Returns the bitrate
    int getBitrate() const;
    // Returns the bitrate as a string
    QString getBitrateStr() const;
    // Sets the bitrate
    void setBitrate(int);
    // Set sample rate
    void setSampleRate(int iSampleRate);
    // Get sample rate
    int getSampleRate() const;
    // Set number of channels
    void setChannels(int iChannels);
    // Get number of channels
    int getChannels() const;
    // Output a formatted string with all the info
    QString getInfo() const;

    QDateTime getDateAdded() const;
    void setDateAdded(const QDateTime& dateAdded);

    // Returns file modified datetime. Limited by the accuracy of what Qt
    // QFileInfo gives us.
    QDateTime getFileModifiedTime() const;

    // Returns file creation datetime. Limited by the accuracy of what Qt
    // QFileInfo gives us.
    QDateTime getFileCreationTime() const;

    // Getter/Setter methods for metadata
    // Return title
    QString getTitle() const;
    // Set title
    void setTitle(const QString&);
    // Return artist
    QString getArtist() const;
    // Set artist
    void setArtist(const QString&);
    // Return album
    QString getAlbum() const;
    // Set album
    void setAlbum(const QString&);
    // Return album artist
    QString getAlbumArtist() const;
    // Set album artist
    void setAlbumArtist(const QString&);
    // Return Year
    QString getYear() const;
    // Set year
    void setYear(const QString&);
    // Return genre
    QString getGenre() const;
    // Set genre
    void setGenre(const QString&);
    // Return composer
    QString getComposer() const;
    // Set composer
    void setComposer(const QString&);
    // Return grouping
    QString getGrouping() const;
    // Set grouping
    void setGrouping(const QString&);
    // Return Track Number
    QString getTrackNumber() const;
    // Set Track Number
    void setTrackNumber(const QString&);
    // Return number of times the track has been played
    int getTimesPlayed() const;
    // Set number of times the track has been played
    void setTimesPlayed(int t);
    // Increment times played with one
    void incTimesPlayed();
    // Returns true if track has been played this instance
    bool getPlayed() const;
    // Set played status and increment or decrement playcount.
    void setPlayedAndUpdatePlaycount(bool);
    // Set played status without affecting the playcount
    void setPlayed(bool bPlayed);
    int getId() const;
    // Returns rating
    int getRating() const;
    // Sets rating
    void setRating(int);
    // Get URL for track
    QString getURL();
    // Set URL for track
    void setURL(const QString& url);
    ConstWaveformPointer getWaveform();
    void setWaveform(ConstWaveformPointer pWaveform);
    ConstWaveformPointer getWaveformSummary() const;
    void setWaveformSummary(ConstWaveformPointer pWaveform);
    void setAnalyserProgress(float progress);
    float getAnalyserProgress() const;
    /** Save the cue point (in samples... I think) */
    void setCuePoint(float cue);
    // Get saved the cue point
    float getCuePoint();
    // Calls for managing the track's cue points
    QSharedPointer<Cue> addCue();
    void removeCue(QSharedPointer<Cue> &cue);
    const QList<QSharedPointer<Cue> > getCuePoints();
    void setCuePoints(QList<QSharedPointer<Cue> > cuePoints);
    bool isDirty();
    // Returns true if the track location has changed
    bool locationChanged();
    // Set the track's full file path
    void setLocation(const QString& location);
    // Get the track's Beats list
    BeatsPointer getBeats() const;
    // Set the track's Beats
    void setBeats(BeatsPointer beats);
    void setKeys(Keys keys);
    const Keys& getKeys() const;
    double getNumericKey() const;
    mixxx::track::io::key::ChromaticKey getKey() const;
    QString getKeyText() const;
    void setKey(mixxx::track::io::key::ChromaticKey key,mixxx::track::io::key::Source source);
    void setKeyText(QString key,mixxx::track::io::key::Source source=mixxx::track::io::key::USER);
    void setCoverInfo(const CoverInfo& cover);
    CoverInfo getCoverInfo() const;
    void setCoverArt(const CoverArt& cover);
    CoverArt getCoverArt() const;
    // Called when the shared pointer reference count for a library TrackPointer
    // drops to zero.
    static void onTrackReferenceExpired(TrackInfoObject* pTrack);
    // Set whether the track should delete itself when its reference count drops
    // to zero. This happens during shutdown when TrackDAO has already been
    // destroyed.
    void setDeleteOnReferenceExpiration(bool deleteOnReferenceExpiration);
  public slots:
    void slotCueUpdated();
  signals:
    void waveformUpdated();
    void waveformSummaryUpdated();
    void coverArtUpdated();
    void analyserProgress(int progress);
    void bpmUpdated(double bpm);
    void beatsUpdated();
    void keyUpdated(double key);
    void keysUpdated();
    void ReplayGainUpdated(double replaygain);
    void cuesUpdated();
    void changed(TrackInfoObject* pTrack);
    void dirty(TrackInfoObject* pTrack);
    void clean(TrackInfoObject* pTrack);
    void referenceExpired(TrackInfoObject* pTrack);
  private slots:
    void slotBeatsUpdated();
  private:
    // Common initialization function between all TIO constructors.
    void initialize(bool parseHeader, bool parseCoverArt);
    void setMetadata(const Mixxx::TrackMetadata& trackMetadata);
    void getMetadata(Mixxx::TrackMetadata* pTrackMetadata);
    // Set whether the TIO is dirty not. This should never be called except by
    // TIO local methods or the TrackDAO.
    void setDirty(bool bDirty);
    // Set a unique identifier for the track. Only used by services like
    // TrackDAO
    void setId(int iId);
    // Whether the track should delete itself when its reference count drops to
    // zero. Used for cleaning up after shutdown.
    std::atomic<bool> m_bDeleteOnReferenceExpiration;
    // Flag that indicates whether or not the TIO has changed. This is used by
    // TrackDAO to determine whether or not to write the Track back.
    std::atomic<bool> m_bDirty;
    // Special flag for telling if the track location was changed.
    std::atomic<bool> m_bLocationChanged;
    // The file
    QFileInfo m_fileInfo;
    SecurityTokenPointer m_pSecurityToken;
    // Metadata
    // Album
    QString m_sAlbum;
    // Artist
    QString m_sArtist;
    // Album Artist
    QString m_sAlbumArtist;
    // Title
    QString m_sTitle;
    // Genre
    QString m_sGenre;
    // Composer
    QString m_sComposer;
    // Grouping
    QString m_sGrouping;
    // Year
    QString m_sYear;
    // Track Number
    QString m_sTrackNumber;
    // File type
    QString m_sType;
    // User comment
    QString m_sComment;
    // URL (used in promo track)
    QString m_sURL;
    // Duration of track in seconds
    std::atomic<float> m_fDuration;
    // Sample rate
    std::atomic<int> m_iSampleRate;
    // Number of channels
    std::atomic<int> m_iChannels;
    // Track rating
    std::atomic<int> m_Rating;
    // Bitrate, number of kilobits per second of audio in the track
    std::atomic<int> m_iBitrate;
    // Number of times the track has been played
    std::atomic<int> m_iTimesPlayed;
    // Replay Gain volume
    std::atomic<float> m_fReplayGain;
    // Has this track been played this sessions?
    std::atomic<bool> m_bPlayed;
    // True if header was parsed
    std::atomic<bool> m_bHeaderParsed;
    // Id. Unique ID of track
    std::atomic<int> m_iId;
    // Cue point in samples or something
    std::atomic<float> m_fCuePoint;
    // Date the track was added to the library
    QDateTime m_dateAdded;
    Keys m_keys;
    // BPM lock
    std::atomic<bool> m_bBpmLock;
    // The list of cue points for the track
    QList<QSharedPointer<Cue> > m_cuePoints;
    // Mutex protecting access to object
    mutable QMutex m_qMutex;
    // Storage for the track's beats
    BeatsPointer m_pBeats;
    //Visual waveform data
    ConstWaveformPointer m_waveform;
    ConstWaveformPointer m_waveformSummary;
    std::atomic<float> m_analyserProgress; // in 0.1%
    CoverArt m_coverArt;
    friend class TrackDAO;
    friend class AutoDJProcessorTest;
};
#endif
