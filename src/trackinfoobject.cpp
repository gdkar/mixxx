/***************************************************************************
                          trackinfoobject.cpp  -  description
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

#include <QDomNode>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QDirIterator>
#include <QFile>
#include <QMutexLocker>
#include <QString>
#include <QtDebug>
#include <QRegExp>

#include "trackinfoobject.h"

#include "controlobject.h"
#include "soundsourceproxy.h"
#include "metadata/trackmetadata.h"
#include "util/xml.h"
#include "track/beatfactory.h"
#include "track/keyfactory.h"
#include "track/keyutils.h"
#include "util/compatibility.h"
#include "util/cmdlineargs.h"
#include "util/time.h"
#include "util/math.h"
#include "waveform/waveform.h"
#include "library/coverartutils.h"
#include "util/assert.h"

TrackInfoObject::TrackInfoObject(const QString& file,
                                 SecurityTokenPointer pToken,
                                 bool parseHeader, bool parseCoverArt)
        : m_fileInfo(file),
          m_pSecurityToken(pToken.isNull() ? Sandbox::openSecurityToken(
                  m_fileInfo, true) : pToken),
          m_qMutex(QMutex::Recursive),
          m_analyserProgress(-1) {
    initialize(parseHeader, parseCoverArt);
}
TrackInfoObject::TrackInfoObject(const QFileInfo& fileInfo,
                                 SecurityTokenPointer pToken,
                                 bool parseHeader, bool parseCoverArt)
        : m_fileInfo(fileInfo),
          m_pSecurityToken(pToken.isNull() ? Sandbox::openSecurityToken(
                  m_fileInfo, true) : pToken),
          m_qMutex(QMutex::Recursive),
          m_analyserProgress(-1) {
    initialize(parseHeader, parseCoverArt);
}
TrackInfoObject::TrackInfoObject(const QDomNode &nodeHeader)
        : m_qMutex(QMutex::Recursive),
          m_analyserProgress(-1) {
    QString filename = XmlParse::selectNodeQString(nodeHeader, "Filename");
    QString location = QDir(XmlParse::selectNodeQString(nodeHeader, "Filepath")).filePath(filename);
    m_fileInfo = QFileInfo(location);
    m_pSecurityToken = Sandbox::openSecurityToken(m_fileInfo, true);

    // We don't call initialize() here because it would end up calling parse()
    // on the file. Plus those initializations weren't done before, so it might
    // cause subtle bugs. This constructor is only used for legacy importing so
    // I'm not going to do it. rryan 6/2010

    m_sTitle = XmlParse::selectNodeQString(nodeHeader, "Title");
    m_sArtist = XmlParse::selectNodeQString(nodeHeader, "Artist");
    m_sType = XmlParse::selectNodeQString(nodeHeader, "Type");
    m_sComment = XmlParse::selectNodeQString(nodeHeader, "Comment");
    m_fDuration = XmlParse::selectNodeQString(nodeHeader, "Duration").toFloat();
    m_iSampleRate = XmlParse::selectNodeQString(nodeHeader, "SampleRate").toInt();
    m_iChannels = XmlParse::selectNodeQString(nodeHeader, "Channels").toInt();
    m_iBitrate = XmlParse::selectNodeQString(nodeHeader, "Bitrate").toInt();
    m_iTimesPlayed = XmlParse::selectNodeQString(nodeHeader, "TimesPlayed").toInt();
    m_fReplayGain = XmlParse::selectNodeQString(nodeHeader, "replaygain").toFloat();
    m_bHeaderParsed = false;
    m_bBpmLock = false;
    m_Rating = 0;
    // Mixxx <1.8 recorded track IDs in mixxxtrack.xml, but we are going to
    // ignore those. Tracks will get a new ID from the database.
    //m_iId = XmlParse::selectNodeQString(nodeHeader, "Id").toInt();
    m_iId = -1;
    m_fCuePoint = XmlParse::selectNodeQString(nodeHeader, "CuePoint").toFloat();
    m_bPlayed = false;
    m_bDeleteOnReferenceExpiration = false;
    m_bDirty = false;
    m_bLocationChanged = false;
}

void TrackInfoObject::initialize(bool parseHeader, bool parseCoverArt) {
    m_bDeleteOnReferenceExpiration = false;
    m_bDirty = false;
    m_bLocationChanged = false;

    m_sArtist = "";
    m_sTitle = "";
    m_sType= "";
    m_sComment = "";
    m_sYear = "";
    m_sURL = "";
    m_fDuration = 0;
    m_iBitrate = 0;
    m_iTimesPlayed = 0;
    m_bPlayed = false;
    m_fReplayGain = 0.;
    m_bHeaderParsed = false;
    m_iId = -1;
    m_iSampleRate = 0;
    m_iChannels = 0;
    m_fCuePoint = 0.0f;
    m_dateAdded = QDateTime::currentDateTime();
    m_Rating = 0;
    m_bBpmLock = false;
    m_sGrouping = "";
    m_sAlbumArtist = "";
    // parse() parses the metadata from file. This is not a quick operation!
    if (parseHeader) {parse(parseCoverArt);}
}
TrackInfoObject::~TrackInfoObject() {
    // qDebug() << "~TrackInfoObject"
    //          << this << m_iId << getInfo();
}
// static
void TrackInfoObject::onTrackReferenceExpired(TrackInfoObject* pTrack) {
    DEBUG_ASSERT_AND_HANDLE(pTrack != nullptr) {return;}
    // qDebug() << "TrackInfoObject::onTrackReferenceExpired"
    //          << pTrack << pTrack->getId() << pTrack->getInfo();
    if (pTrack->m_bDeleteOnReferenceExpiration) {
        delete pTrack;
    } else {
        emit(pTrack->referenceExpired(pTrack));
    }
}
void TrackInfoObject::setDeleteOnReferenceExpiration(bool deleteOnReferenceExpiration) {
    m_bDeleteOnReferenceExpiration = deleteOnReferenceExpiration;
}
namespace {
    // Parses artist/title from the file name and returns the file type.
    // Assumes that the file name is written like: "artist - title.xxx"
    // or "artist_-_title.xxx",
    void parseMetadataFromFileName(Mixxx::TrackMetadata& trackMetadata, QString fileName) {
        fileName.replace("_", " ");
        QString titleWithFileType;
        if (fileName.count('-') == 1) {
            const QString artist(fileName.section('-', 0, 0).trimmed());
            if (!artist.isEmpty()) {trackMetadata.setArtist(artist);}
            titleWithFileType = fileName.section('-', 1, 1).trimmed();
        } else {
            titleWithFileType = fileName.trimmed();
        }
        const QString title(titleWithFileType.section('.', 0, -2).trimmed());
        if (!title.isEmpty()) {
            trackMetadata.setTitle(title);
        }
    }
}

void TrackInfoObject::setMetadata(const Mixxx::TrackMetadata& trackMetadata) {
    // TODO(XXX): This involves locking the mutex for every setXXX
    // method. We should figure out an optimization where there are private
    // setters that don't lock the mutex.
    setArtist(trackMetadata.getArtist());
    setTitle(trackMetadata.getTitle());
    setAlbum(trackMetadata.getAlbum());
    setAlbumArtist(trackMetadata.getAlbumArtist());
    setYear(trackMetadata.getYear());
    setGenre(trackMetadata.getGenre());
    setComposer(trackMetadata.getComposer());
    setGrouping(trackMetadata.getGrouping());
    setComment(trackMetadata.getComment());
    setTrackNumber(trackMetadata.getTrackNumber());
    setChannels(trackMetadata.getChannels());
    setSampleRate(trackMetadata.getSampleRate());
    setDuration(trackMetadata.getDuration());
    setBitrate(trackMetadata.getBitrate());

    if (trackMetadata.isReplayGainValid()) {setReplayGain(trackMetadata.getReplayGain());}

    // Need to set BPM after sample rate since beat grid creation depends on
    // knowing the sample rate. Bug #1020438.
    if (trackMetadata.isBpmValid()) {setBpm(trackMetadata.getBpm());}

    const QString key(trackMetadata.getKey());
    if (!key.isEmpty()) {setKeyText(key, mixxx::track::io::key::FILE_METADATA);}
}

void TrackInfoObject::getMetadata(Mixxx::TrackMetadata* pTrackMetadata) {
    // TODO(XXX): This involves locking the mutex for every setXXX
    // method. We should figure out an optimization where there are private
    // getters that don't lock the mutex.
    pTrackMetadata->setArtist(getArtist());
    pTrackMetadata->setTitle(getTitle());
    pTrackMetadata->setAlbum(getAlbum());
    pTrackMetadata->setAlbumArtist(getAlbumArtist());
    pTrackMetadata->setYear(Mixxx::TrackMetadata::reformatYear(getYear()));
    pTrackMetadata->setGenre(getGenre());
    pTrackMetadata->setComposer(getComposer());
    pTrackMetadata->setGrouping(getGrouping());
    pTrackMetadata->setComment(getComment());
    pTrackMetadata->setTrackNumber(getTrackNumber());
    pTrackMetadata->setChannels(getChannels());
    pTrackMetadata->setSampleRate(getSampleRate());
    pTrackMetadata->setDuration(getDuration());
    pTrackMetadata->setBitrate(getBitrate());
    pTrackMetadata->setReplayGain(getReplayGain());
    pTrackMetadata->setBpm(getBpm());
    pTrackMetadata->setKey(getKeyText());
}

void TrackInfoObject::parse(bool parseCoverArt) {
    // Log parsing of header information in developer mode. This is useful for
    // tracking down corrupt files.
    const QString& canonicalLocation = m_fileInfo.canonicalFilePath();
    if (CmdlineArgs::Instance().getDeveloper()) {
        qDebug() << "TrackInfoObject::parse()" << canonicalLocation;
    }

    SoundSourceProxy proxy(canonicalLocation, m_pSecurityToken);
    if (!proxy.getType().isEmpty()) {
        setType(proxy.getType());

        // Parse the information stored in the sound file.
        Mixxx::TrackMetadata trackMetadata;
        QImage coverArt;
        // If parsing of the cover art image should be omitted the
        // 2nd output parameter must be set to nullptr.
        QImage* pCoverArt = parseCoverArt ? &coverArt : nullptr;
        if (proxy.parseTrackMetadataAndCoverArt(&trackMetadata, pCoverArt) == OK) {
            // If Artist, Title and Type fields are not blank, modify them.
            // Otherwise, keep their current values.
            // TODO(rryan): Should we re-visit this decision?
            if (trackMetadata.getArtist().isEmpty() || trackMetadata.getTitle().isEmpty()) {
                Mixxx::TrackMetadata fileNameMetadata;
                parseMetadataFromFileName(fileNameMetadata, m_fileInfo.fileName());
                if (trackMetadata.getArtist().isEmpty()) {
                    trackMetadata.setArtist(fileNameMetadata.getArtist());
                }
                if (trackMetadata.getTitle().isEmpty()) {
                    trackMetadata.setTitle(fileNameMetadata.getTitle());
                }
            }

            if (pCoverArt && !pCoverArt->isNull()) {
                QMutexLocker lock(&m_qMutex);
                m_coverArt.image = *pCoverArt;
                m_coverArt.info.hash = CoverArtUtils::calculateHash(
                    m_coverArt.image);
                m_coverArt.info.coverLocation = QString();
                m_coverArt.info.type = CoverInfo::METADATA;
                m_coverArt.info.source = CoverInfo::GUESSED;
            }

            setHeaderParsed(true);
        } else {
            qDebug() << "TrackInfoObject::parse() error at file"
                     << canonicalLocation;

            // Add basic information derived from the filename
            parseMetadataFromFileName(trackMetadata, m_fileInfo.fileName());

            setHeaderParsed(false);
        }
        // Dump the metadata extracted from the file into the track.
        setMetadata(trackMetadata);
    } else {
        qDebug() << "TrackInfoObject::parse() error at file"
                 << canonicalLocation;
        setHeaderParsed(false);
    }
}

QString TrackInfoObject::getDurationStr() const {
    return Time::formatSeconds(getDuration(), false);
}

void TrackInfoObject::setLocation(const QString& location) {
    QFileInfo newFileInfo(location);
    QFileInfo oldFileInfo(newFileInfo);
    qSwap(m_fileInfo,oldFileInfo);
    if (newFileInfo != oldFileInfo) {
        m_bLocationChanged.store(true);
        setDirty(true);
        emit changed(this);
    }
}

QString TrackInfoObject::getLocation() const {
    return getFileInfo().absoluteFilePath();
}

QString TrackInfoObject::getCanonicalLocation() const {
    return getFileInfo().canonicalFilePath();
}

QFileInfo TrackInfoObject::getFileInfo() const {
    // No need for locking since we are passing a copy by value. Qt doesn't say
    // that QFileInfo is thread-safe but its copy constructor just copies the
    // d_ptr
    QFileInfo oldFileInfo(m_fileInfo);
    return oldFileInfo;
}

SecurityTokenPointer TrackInfoObject::getSecurityToken() {
    return m_pSecurityToken;
}

QString TrackInfoObject::getDirectory() const {
    return getFileInfo().absolutePath();
}

QString TrackInfoObject::getFilename() const {
    return getFileInfo().fileName();
}

bool TrackInfoObject::exists() const {
    // return here a fresh calculated value to be sure
    // the file is not deleted or gone with an USB-Stick
    // because it will probably stop the Auto-DJ
    return QFile::exists(getFileInfo().absoluteFilePath());
}

float TrackInfoObject::getReplayGain() const {
    return m_fReplayGain.load();
}
void TrackInfoObject::setReplayGain(float f) {
    //qDebug() << "Reported ReplayGain value: " << m_fReplayGain;
    if (m_fReplayGain.exchange(f) != f) {
        setDirty(true);
        emit(ReplayGainUpdated(f));
        emit(changed(this));
    }
}
double TrackInfoObject::getBpm() const {
    QMutexLocker lock(&m_qMutex);
    if (!m_pBeats) {
        return 0;
    }
    // getBpm() returns -1 when invalid.
    double bpm = m_pBeats->getBpm();
    if (bpm >= 0.0) {return bpm;}
    return 0;
}
void TrackInfoObject::setBpm(double f) {
    if (f < 0) {return;}
    QMutexLocker lock(&m_qMutex);
    // TODO(rryan): Assume always dirties.
    bool dirty = false;
    if (f == 0.0) {
        // If the user sets the BPM to 0, we assume they want to clear the
        // beatgrid.
        setBeats(BeatsPointer());
        dirty = true;
    } else if (!m_pBeats) {
        setBeats(BeatFactory::makeBeatGrid(this, f, 0));
        dirty = true;
    } else if (m_pBeats->getBpm() != f) {
        m_pBeats->setBpm(f);
        dirty = true;
    }
    if (dirty) {setDirty(true);}
    lock.unlock();
    // Tell the GUI to update the bpm label...
    //qDebug() << "TrackInfoObject signaling BPM update to" << f;
    emit(bpmUpdated(f));
    emit changed(this);
}

QString TrackInfoObject::getBpmStr() const
{
    return QString("%1").arg(getBpm(), 3,'f',1);
}

void TrackInfoObject::setBeats(BeatsPointer pBeats) {
    QMutexLocker lock(&m_qMutex);

    // This whole method is not so great. The fact that Beats is an ABC is
    // limiting with respect to QObject and signals/slots.

    QObject* pObject = nullptr;
    if (m_pBeats) {
        pObject = dynamic_cast<QObject*>(m_pBeats.data());
        if (pObject) {
            disconnect(pObject, SIGNAL(updated()),this, SLOT(slotBeatsUpdated()));
        }
    }
    m_pBeats = pBeats;
    double bpm = 0.0;
    if (m_pBeats) {
        bpm = m_pBeats->getBpm();
        pObject = dynamic_cast<QObject*>(m_pBeats.data());
        if (pObject) {
            connect(pObject, SIGNAL(updated()),this, SLOT(slotBeatsUpdated()));
        }
    }
    setDirty(true);
    lock.unlock();
    emit(bpmUpdated(bpm));
    emit(beatsUpdated());
}

BeatsPointer TrackInfoObject::getBeats() const {
    QMutexLocker lock(&m_qMutex);
    return m_pBeats;
}

void TrackInfoObject::slotBeatsUpdated() {
    QMutexLocker lock(&m_qMutex);
    setDirty(true);
    double bpm = m_pBeats->getBpm();
    lock.unlock();
    emit(bpmUpdated(bpm));
    emit(beatsUpdated());
}
bool TrackInfoObject::getHeaderParsed()  const{
    QMutexLocker lock(&m_qMutex);
    return m_bHeaderParsed;
}
void TrackInfoObject::setHeaderParsed(bool parsed){
    QMutexLocker lock(&m_qMutex);
    if (m_bHeaderParsed != parsed) {
        m_bHeaderParsed = parsed;
        setDirty(true);
    }
}
QString TrackInfoObject::getInfo()  const{
    QString sArtist(m_sArtist);
    QString sTitle(m_sTitle);
    QString artist = sArtist.trimmed() == "" ? "" : sArtist + ", ";
    QString sInfo = artist + sTitle;
    return sInfo;
}
QDateTime TrackInfoObject::getDateAdded() const {
    QDateTime dateAdded(m_dateAdded);
    return dateAdded;
}
void TrackInfoObject::setDateAdded(const QDateTime& dateAdded) {
    QDateTime oldDateAdded(dateAdded);
    qSwap(m_dateAdded,oldDateAdded);
    if(oldDateAdded!=m_dateAdded){
        emit changed(this);
    }
}
QDateTime TrackInfoObject::getFileModifiedTime() const {return getFileInfo().lastModified();}
QDateTime TrackInfoObject::getFileCreationTime() const {
    return getFileInfo().created();
}
float TrackInfoObject::getDuration()  const {return m_fDuration.load();}
void TrackInfoObject::setDuration(float i) {
    if (m_fDuration.exchange(i) != i) {setDirty(true);emit changed(this);}
}
QString TrackInfoObject::getTitle() const {
    QString sTitle(m_sTitle);
    return sTitle;
}
void TrackInfoObject::setTitle(const QString& s) {
    QString title = s.trimmed();
    QString oldTitle(title);
    qSwap(m_sTitle,oldTitle);
    if (oldTitle != title) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getArtist() const {
    QString sArtist(m_sArtist);
    return sArtist;
}
void TrackInfoObject::setArtist(const QString& s) {
    QString artist = s.trimmed();
    QString oldArtist(artist);
    qSwap(m_sArtist,oldArtist);
    if (oldArtist != artist) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getAlbum() const {
    QString album(m_sAlbum);
    return album;
}
void TrackInfoObject::setAlbum(const QString& s) {
    QString album = s.trimmed();
    QString old(album);
    m_sAlbum.swap(old);
    if (old != album) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getAlbumArtist()  const {
    QString old(m_sAlbumArtist);
    return old;
}
void TrackInfoObject::setAlbumArtist(const QString& s) {
    QString st = s.trimmed();
    QString old(st);
    m_sAlbumArtist.swap(old);
    if (old != st) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getYear()  const {
    QString old(m_sYear);
    return old;
}
void TrackInfoObject::setYear(const QString& s) {
    QString year = s.trimmed();
    QString old(year);
    m_sYear.swap(old);
    if (old != year) {
        setDirty(true);
        emit changed(this);

    }
}
QString TrackInfoObject::getGenre() const {
    QString genre(m_sGenre);
    return genre;
}
void TrackInfoObject::setGenre(const QString& s) {
    QString genre = s.trimmed();
    QString old(genre);
    m_sGenre.swap(old);
    if (old != genre) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getComposer() const {
    QString old(m_sComposer);
    return old;
}
void TrackInfoObject::setComposer(const QString& s) {
    QString composer = s.trimmed();
    QString old(composer);
    m_sComposer.swap(old);
    if (old != composer) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getGrouping()  const {
    QString old(m_sGrouping);
    return old;
}
void TrackInfoObject::setGrouping(const QString& s) {
    QString grouping = s.trimmed();
    QString old(grouping);
    m_sGrouping.swap(old);
    if (old != grouping) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getTrackNumber()  const {
    QString tn(m_sTrackNumber);
    return tn;
}
void TrackInfoObject::setTrackNumber(const QString& s) {
    QString tn = s.trimmed();
    QString oldTn(tn);
    qSwap(m_sTrackNumber,oldTn);
    if (oldTn != tn) {
        setDirty(true);
        emit changed(this);
    }
}
int TrackInfoObject::getTimesPlayed() const {return m_iTimesPlayed.load();}
void TrackInfoObject::setTimesPlayed(int t) {
    if (t != m_iTimesPlayed.exchange(t)) {
        setDirty(true);
        emit changed(this);
    }
}
void TrackInfoObject::incTimesPlayed() {setPlayedAndUpdatePlaycount(true);}
bool TrackInfoObject::getPlayed() const {return m_bPlayed.load();}
void TrackInfoObject::setPlayedAndUpdatePlaycount(bool bPlayed) {
  bool changed = m_bPlayed.exchange(bPlayed)!=bPlayed;
    if (bPlayed) {
        m_iTimesPlayed++;
        changed=true;
    }
    else if (changed) {
        m_iTimesPlayed--;
    }
    if(changed){
      setDirty(true);
      emit changed(this);
    }
}
void TrackInfoObject::setPlayed(bool bPlayed) {
    if (bPlayed != m_bPlayed.exchange(bPlayed)) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getComment() const {
    QString sComment(m_sComment);
    return sComment;
}
void TrackInfoObject::setComment(const QString& s) {
    QString sComment(s);
    qSwap(sComment,m_sComment)
    if (s != sComment) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getType() const {
    auto sType = m_sType
    return sType;
}
void TrackInfoObject::setType(const QString& s) {
    QString sType(s);
    qSwap(sType,m_sType);
    if (sType != s) {
        setDirty(true);
        emit changed(this);
    }
}
void TrackInfoObject::setSampleRate(int iSampleRate) {
    if (m_iSampleRate.exchange(iSampleRate) != iSampleRate) {
        setDirty(true);
        emit changed(this);
    }
}
int TrackInfoObject::getSampleRate() const {
    return m_iSampleRate.load();
}
void TrackInfoObject::setChannels(int iChannels) {
    if (m_iChannels.exchange(iChannels) != iChannels) {
        setDirty(true);
        emit changed(this);
    }
}
int TrackInfoObject::getChannels() const {
    return m_iChannels.load();
}

int TrackInfoObject::getLength() const {
    return getFileInfo().size();
}

int TrackInfoObject::getBitrate() const {
    return m_iBitrate.load();
}
QString TrackInfoObject::getBitrateStr() const {
    return QString("%1").arg(getBitrate());
}
void TrackInfoObject::setBitrate(int i) {
    if (m_iBitrate.exchange(i) != i) {
        setDirty(true);
        emit changed(this);
    }
}
int TrackInfoObject::getId() const {return m_iId.load();}
void TrackInfoObject::setId(int iId) {
    // changing the Id does not make the track dirty because the Id is always
    // generated by the Database itself
    m_iId.store(iId);
}
//TODO (vrince) remove clean-up when new summary is ready
/*
const QByteArray *TrackInfoObject::getWaveSummary()
{
    QMutexLocker lock(&m_qMutex);
    return &m_waveSummary;
}

void TrackInfoObject::setWaveSummary(const QByteArray* pWave, bool updateUI)
{
    QMutexLocker lock(&m_qMutex);
    m_waveSummary = *pWave; //_Copy_ the bytes
    setDirty(true);
    lock.unlock();
    emit(wavesummaryUpdated(this));
}*/

void TrackInfoObject::setURL(const QString& url) {
    QString sURL(url);
    m_sURL.swap(sURL);
    if (sURL != url) {
        setDirty(true);
        emit changed(this);
    }
}
QString TrackInfoObject::getURL() {
    auto sURL = m_sURL;
    return sURL;
}
ConstWaveformPointer TrackInfoObject::getWaveform() {return m_waveform;}
void TrackInfoObject::setWaveform(ConstWaveformPointer pWaveform) {
    if(atomic_exchange(m_waveform,pWaveform)!=pWaveform){
      emit(waveformUpdated());
    }
}
ConstWaveformPointer TrackInfoObject::getWaveformSummary() const {return m_waveformSummary;}
void TrackInfoObject::setWaveformSummary(ConstWaveformPointer pWaveform) {
    if(atomic_exchange(m_waveformSummary, pWaveform)!=pWaveform){
      emit(waveformSummaryUpdated());
    }
}
void TrackInfoObject::setAnalyserProgress(float progress) {
    // progress in 0 .. 1000. 
    if (m_analyserProgress.exchange(progress)!=progress) {
        emit(analyserProgress(progress));
    }
}
float TrackInfoObject::getAnalyserProgress() const {return m_analyserProgress.load();}
void TrackInfoObject::setCuePoint(float cue) {
    if (m_fCuePoint.exchange(cue) != cue) {
        setDirty(true);
        emit(changed(this));
    }
}
float TrackInfoObject::getCuePoint() {return m_fCuePoint.load();}
void TrackInfoObject::slotCueUpdated() {
    setDirty(true);
    emit(cuesUpdated());
}
QSharedPointer<Cue> TrackInfoObject::addCue() {
    //qDebug() << "TrackInfoObject::addCue()";
    auto cue = QSharedPointer<Cue>(new Cue(m_iId));
    QList<QSharedPointer<Cue> > oldList(m_cuePoints);
    oldList.push_back(QSharedPointer<Cue>(new Cue(m_iId)));
    connect(oldList.back().data(), &Cue::updated,this, &TrackInfoObject::slotCueUpdated,
        static_cast<Qt::ConnectionType>(Qt::QueuedConnection|Qt::UniqueConnection));
    m_cuePoints.swap(oldList);
    setDirty(true);
    emit(cuesUpdated());
    return oldList.back();
}
void TrackInfoObject::removeCue(QSharedPointer<Cue> &cue) {
    QMutexLocker lock(&m_qMutex);
    disconnect(cue.data(), 0, this, 0);
    // TODO(XXX): Delete the cue point.
    QList<QSharedPointer<Cue> > oldList(m_cuePoints);
    oldList.removeOne(cue);
    m_cuePoints.swap(oldList);
    setDirty(true);
    emit(cuesUpdated());
}
const QList<QSharedPointer<Cue> > TrackInfoObject::getCuePoints() {
    auto oldList = m_cuePoints;
    return oldList;
}
void TrackInfoObject::setCuePoints(QList<QSharedPointer<Cue> > cuePoints) {
    //qDebug() << "setCuePoints" << cuePoints.length();
    auto oldPoints = QList<QSharedPointer<Cue> >(m_cuePoints);
    for(auto &cue : oldPoints){
      disconnect(cue.data(),0,this,0);
    }
    for(auto &cue : m_cuePoints){
      connect(cue,&Cue::updated,this,&TrackInfoObject::slotCueUpdated,
          static_cast<Qt::ConnectionType>(Qt::UniqueConnection|Qt::QueuedConnection));
    }
    m_cuePoints.swap(cuePoints);
    setDirty(true);
    emit(cuesUpdated());
}
void TrackInfoObject::setDirty(bool bDirty) {
    if(m_bDirty.exchange(bDirty)!=bDirty){
      if(bDirty) emit(dirty(this));
      else       emit(clean(this));
      emit(changed(this));
    }
}
bool TrackInfoObject::isDirty() {        return m_bDirty.load();}
bool TrackInfoObject::locationChanged() {return m_bLocationChanged.load();}
int TrackInfoObject::getRating() const { return m_Rating.load();}
void TrackInfoObject::setRating (int rating) {
    if(m_Rating.exchange(rating)!=rating){
        setDirty(true);
        emit changed(this);
    }
}
void TrackInfoObject::setKeys(Keys keys) {
    QMutexLocker lock(&m_qMutex);
    setDirty(true);
    m_keys = keys;
    // Might be INVALID. We don't care.
    mixxx::track::io::key::ChromaticKey newKey = m_keys.getGlobalKey();
    lock.unlock();
    emit(keyUpdated(KeyUtils::keyToNumericValue(newKey)));
    emit(keysUpdated());
}
const Keys& TrackInfoObject::getKeys() const {
    QMutexLocker lock(&m_qMutex);
    return m_keys;
}
mixxx::track::io::key::ChromaticKey TrackInfoObject::getKey() const {
    QMutexLocker lock(&m_qMutex);
    if (!m_keys.isValid()) {return mixxx::track::io::key::INVALID;}
    return m_keys.getGlobalKey();
}
void TrackInfoObject::setKey(mixxx::track::io::key::ChromaticKey key,mixxx::track::io::key::Source source) {
    QMutexLocker lock(&m_qMutex);
    bool dirty = false;
    if (key == mixxx::track::io::key::INVALID) {
        m_keys = Keys();
        dirty = true;
    } else if (m_keys.getGlobalKey() != key) {
        m_keys = KeyFactory::makeBasicKeys(key, source);
    }
    if (dirty) {setDirty(true);}
    // Might be INVALID. We don't care.
    mixxx::track::io::key::ChromaticKey newKey = m_keys.getGlobalKey();
    lock.unlock();
    emit(keyUpdated(KeyUtils::keyToNumericValue(newKey)));
    emit(keysUpdated());
}
void TrackInfoObject::setKeyText(QString key,mixxx::track::io::key::Source source) {
    QMutexLocker lock(&m_qMutex);
    Keys newKeys = KeyFactory::makeBasicKeysFromText(key, source);
    // We treat this as dirtying if it is parsed to a different key or if we
    // fail to parse the key, if the text value is different from the current
    // text value.
    bool dirty = newKeys.getGlobalKey() != m_keys.getGlobalKey() ||
            (newKeys.getGlobalKey() == mixxx::track::io::key::INVALID &&
             newKeys.getGlobalKeyText() != m_keys.getGlobalKeyText());
    if (dirty) {
        m_keys = newKeys;
        setDirty(true);
        // Might be INVALID. We don't care.
        mixxx::track::io::key::ChromaticKey newKey = m_keys.getGlobalKey();
        lock.unlock();
        emit(keyUpdated(KeyUtils::keyToNumericValue(newKey)));
        emit(keysUpdated());
    }
}
QString TrackInfoObject::getKeyText() const {
    QMutexLocker lock(&m_qMutex);
    mixxx::track::io::key::ChromaticKey key = m_keys.getGlobalKey();
    if (key != mixxx::track::io::key::INVALID) {return KeyUtils::keyToString(key);}
    // Fall back on text global name.
    QString keyText = m_keys.getGlobalKeyText();
    return keyText;
}
void TrackInfoObject::setBpmLock(bool bpmLock) {
    if(m_bBpmLock.exchange(bpmLock)!=bpmLock){
        setDirty(true);
        emit changed(this);
    }
}
bool TrackInfoObject::hasBpmLock() const {return m_bBpmLock.load();}
void TrackInfoObject::setCoverInfo(const CoverInfo& info) {
    QMutexLocker lock(&m_qMutex);
    if (info != m_coverArt.info) {
        m_coverArt = CoverArt();
        m_coverArt.info = info;
        setDirty(true);
        lock.unlock();
        emit(coverArtUpdated());
    }
}
CoverInfo TrackInfoObject::getCoverInfo() const {
    QMutexLocker lock(&m_qMutex);
    return m_coverArt.info;
}
void TrackInfoObject::setCoverArt(const CoverArt& cover) {
    QMutexLocker lock(&m_qMutex);
    if (cover != m_coverArt) {
        m_coverArt = cover;
        setDirty(true);
        lock.unlock();
        emit(coverArtUpdated());
    }
}
CoverArt TrackInfoObject::getCoverArt() const {
    QMutexLocker lock(&m_qMutex);
    return m_coverArt;
}
