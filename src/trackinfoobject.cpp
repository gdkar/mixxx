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

TrackInfoObject::TrackInfoObject(const QString& file, SecurityTokenPointer pToken, bool parseHeader, bool parseCoverArt)
        : m_fileInfo(file),
          m_pSecurityToken(pToken.isNull() ? Sandbox::openSecurityToken( m_fileInfo, true) : pToken){
    initialize(parseHeader, parseCoverArt);
}
TrackInfoObject::TrackInfoObject(const QFileInfo& fileInfo, SecurityTokenPointer pToken, bool parseHeader, bool parseCoverArt)
        : m_fileInfo(fileInfo),
          m_pSecurityToken(pToken.isNull() ? Sandbox::openSecurityToken(
                  m_fileInfo, true) : pToken)
{
    initialize(parseHeader, parseCoverArt);
}
TrackInfoObject::TrackInfoObject(const QDomNode &nodeHeader){
    auto filename = XmlParse::selectNodeQString(nodeHeader, "Filename");
    auto  location = QDir(XmlParse::selectNodeQString(nodeHeader, "Filepath")).filePath(filename);
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
    m_dDuration = XmlParse::selectNodeQString(nodeHeader, "Duration").toInt();
    m_iSampleRate = XmlParse::selectNodeQString(nodeHeader, "SampleRate").toInt();
    m_iChannels = XmlParse::selectNodeQString(nodeHeader, "Channels").toInt();
    m_iBitrate = XmlParse::selectNodeQString(nodeHeader, "Bitrate").toInt();
    m_iTimesPlayed = XmlParse::selectNodeQString(nodeHeader, "TimesPlayed").toInt();
    m_dReplayGain = XmlParse::selectNodeQString(nodeHeader, "replaygain").toDouble();
    m_bHeaderParsed = false;
    m_bBpmLock = false;
    m_Rating = 0;
    // Mixxx <1.8 recorded track IDs in mixxxtrack.xml, but we are going to
    // ignore those. Tracks will get a new ID from the database.
    //m_id = XmlParse::selectNodeQString(nodeHeader, "Id").toInt();
    m_id = TrackId();
    m_dCuePoint.store(XmlParse::selectNodeQString(nodeHeader, "CuePoint").toDouble());
    m_bPlayed.store(false);
    m_bDeleteOnReferenceExpiration.store(false);
    m_bDirty.store(false);
    m_bLocationChanged.store(false);
}

void TrackInfoObject::initialize(bool parseHeader, bool parseCoverArt) {
    m_bDeleteOnReferenceExpiration.store(false);
    m_bDirty.store(false);
    m_bLocationChanged.store(false);

    m_sArtist = "";
    m_sTitle = "";
    m_sType= "";
    m_sComment = "";
    m_sYear = "";
    m_sURL = "";
    m_dDuration.store(0);
    m_iBitrate.store(0);
    m_iTimesPlayed.store(0);
    m_bPlayed.store(false);
    m_dReplayGain.store(0.);
    m_bHeaderParsed = false;
    m_id = TrackId();
    m_iSampleRate.store(0);
    m_iChannels.store(0);
    m_dCuePoint.store(0.0);
    m_dateAdded = QDateTime::currentDateTime();
    m_Rating.store(0);
    m_bBpmLock.store(false);
    m_sGrouping = "";
    m_sAlbumArtist = "";
    // parse() parses the metadata from file. This is not a quick operation!
    if (parseHeader) {parse(parseCoverArt);}
}
TrackInfoObject::~TrackInfoObject() {
    // qDebug() << "~TrackInfoObject"
    //          << this << m_id << getInfo();
}

// static
void TrackInfoObject::onTrackReferenceExpired(TrackInfoObject* pTrack) {
    DEBUG_ASSERT_AND_HANDLE(pTrack) {return;}
    // qDebug() << "TrackInfoObject::onTrackReferenceExpired"
    //          << pTrack << pTrack->getId() << pTrack->getInfo();
    if (pTrack->m_bDeleteOnReferenceExpiration) { delete pTrack;}
    else {emit(pTrack->referenceExpired(pTrack));}
}
void TrackInfoObject::setDeleteOnReferenceExpiration(bool deleteOnReferenceExpiration) {
    m_bDeleteOnReferenceExpiration.store(deleteOnReferenceExpiration);
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
        } else {titleWithFileType = fileName.trimmed();}
        const QString title(titleWithFileType.section('.', 0, -2).trimmed());
        if (!title.isEmpty()) {trackMetadata.setTitle(title);}
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
    if (CmdlineArgs::Instance().getDeveloper()) { qDebug() << "TrackInfoObject::parse()" << canonicalLocation;}
    SoundSourceProxy proxy(canonicalLocation, m_pSecurityToken);
    if (!proxy.getType().isEmpty()) {
        setType(proxy.getType());
        // Parse the information stored in the sound file.
        Mixxx::TrackMetadata trackMetadata;
        QImage coverArt;
        // If parsing of the cover art image should be omitted the
        // 2nd output parameter must be set to NULL.
        QImage* pCoverArt = parseCoverArt ? &coverArt : NULL;
        if (proxy.parseTrackMetadataAndCoverArt(&trackMetadata, pCoverArt) == OK) {
            // If Artist, Title and Type fields are not blank, modify them.
            // Otherwise, keep their current values.
            // TODO(rryan): Should we re-visit this decision?
            if (trackMetadata.getArtist().isEmpty() || trackMetadata.getTitle().isEmpty()) {
                Mixxx::TrackMetadata fileNameMetadata;
                parseMetadataFromFileName(fileNameMetadata, m_fileInfo.fileName());
                if (trackMetadata.getArtist().isEmpty()) { trackMetadata.setArtist(fileNameMetadata.getArtist()); }
                if (trackMetadata.getTitle().isEmpty()) { trackMetadata.setTitle(fileNameMetadata.getTitle()); }
            }
            if (pCoverArt && !pCoverArt->isNull()) {
                QMutexLocker lock(&m_qMutex);
                m_coverArt.image = *pCoverArt;
                m_coverArt.info.hash = CoverArtUtils::calculateHash( m_coverArt.image);
                m_coverArt.info.coverLocation = QString();
                m_coverArt.info.type = CoverInfo::METADATA;
                m_coverArt.info.source = CoverInfo::GUESSED;
            }
            setHeaderParsed(true);
        } else {
            qDebug() << "TrackInfoObject::parse() error at file" << canonicalLocation;
            // Add basic information derived from the filename
            parseMetadataFromFileName(trackMetadata, m_fileInfo.fileName());
            setHeaderParsed(false);
        }
        // Dump the metadata extracted from the file into the track.
        setMetadata(trackMetadata);
    } else {
        qDebug() << "TrackInfoObject::parse() error at file" << canonicalLocation;
        setHeaderParsed(false);
    }
}
QString TrackInfoObject::getDurationStr() const {
    auto dDuration = m_dDuration.load();
    return Time::formatSeconds(dDuration, false);
}
void TrackInfoObject::setLocation(const QString& location) {
    auto newFileInfo = QFileInfo{location};
    auto oldFileInfo = newFileInfo;
    qSwap(oldFileInfo, m_fileInfo);
    if (newFileInfo != oldFileInfo) {
        m_bLocationChanged.store(true);
        setDirty(true);
    }
}
QString TrackInfoObject::getLocation() const { return getFileInfo().absoluteFilePath(); }
QString TrackInfoObject::getCanonicalLocation() const { return getFileInfo().canonicalFilePath(); }
QFileInfo TrackInfoObject::getFileInfo() const {
    // No need for locking since we are passing a copy by value. Qt doesn't say
    // that QFileInfo is thread-safe but its copy constructor just copies the
    // d_ptr.
    auto fileInfo = m_fileInfo;
    return fileInfo;
}
SecurityTokenPointer TrackInfoObject::getSecurityToken() { return m_pSecurityToken; }
QString TrackInfoObject::getDirectory() const { return getFileInfo().absolutePath(); }
QString TrackInfoObject::getFilename() const { return getFileInfo().fileName(); }
bool TrackInfoObject::exists() const {
    // return here a fresh calculated value to be sure
    // the file is not deleted or gone with an USB-Stick
    // because it will probably stop the Auto-DJ
    return QFile::exists(getLocation());
}
double TrackInfoObject::getReplayGain() const { return m_dReplayGain.load(); }
void TrackInfoObject::setReplayGain(double f) {
    //qDebug() << "Reported ReplayGain value: " << m_fReplayGain;
    auto g = m_dReplayGain.exchange(f);
    if (g!=f) {
        setDirty(true);
        emit(ReplayGainUpdated(f));
    }
}
double TrackInfoObject::getBpm() const {
    auto pBeats = m_pBeats;
    if (!pBeats) { return 0; }
    // getBpm() returns -1 when invalid.
    auto bpm = pBeats->getBpm();
    if (bpm >= 0.0) { return bpm; }
    return 0;
}
void TrackInfoObject::setBpm(double f) {
    if (f < 0) { return; }
    // TODO(rryan): Assume always dirties.
    auto dirty = false;
    auto pBeats = m_pBeats;
    if (!f) {
        // If the user sets the BPM to 0, we assume they want to clear the
        // beatgrid.
        setBeats(BeatsPointer());
        dirty = true;
    } else if (!pBeats) {
        setBeats(BeatFactory::makeBeatGrid(this, f, 0));
        dirty = true;
    } else if (pBeats->getBpm() != f) {
        pBeats->setBpm(f);
        dirty = true;
    }
    if (dirty) { 
      setDirty(true);
    // Tell the GUI to update the bpm label...
    //qDebug() << "TrackInfoObject signaling BPM update to" << f;
      emit(bpmUpdated(f));
    }
}
QString TrackInfoObject::getBpmStr() const { return QString("%1").arg(getBpm(), 3,'f',1); }
void TrackInfoObject::setBeats(BeatsPointer pBeats) {
    // This whole method is not so great. The fact that Beats is an ABC is
    // limiting with respect to QObject and signals/slots.
    if (m_pBeats) {
        if(auto pObject = dynamic_cast<QObject*>(m_pBeats.data()) )
        { disconnect(pObject, SIGNAL(updated()), this, SLOT(slotBeatsUpdated())); }
    }
    auto oldBeats = pBeats;
    m_pBeats.swap ( oldBeats );
    auto bpm = 0.0;
    if (pBeats) {
        bpm = pBeats->getBpm();
        if(auto pObject = dynamic_cast<QObject*>(pBeats.data()) )
        {connect(pObject, SIGNAL(updated()), this, SLOT(slotBeatsUpdated()));}
    }
    setDirty(true);
    emit(bpmUpdated(bpm));
    emit(beatsUpdated());
}
BeatsPointer TrackInfoObject::getBeats() const  {return m_pBeats; }
void TrackInfoObject::slotBeatsUpdated() {
    setDirty(true);
    auto bpm = m_pBeats->getBpm();
    emit(bpmUpdated(bpm));
    emit(beatsUpdated());
}
bool TrackInfoObject::getHeaderParsed()  const { return m_bHeaderParsed.load(); }
void TrackInfoObject::setHeaderParsed(bool parsed)
{ if ( parsed != m_bHeaderParsed.exchange(parsed )){ setDirty(true); } }
QString TrackInfoObject::getInfo()  const{
    auto sArtist = QString{m_sArtist};
    auto sTitle  = QString{m_sTitle };
    auto artist = sArtist.trimmed() == "" ? "" : sArtist + ", ";
    return  artist + sTitle;
}
QDateTime TrackInfoObject::getDateAdded() const { return m_dateAdded; }
void TrackInfoObject::setDateAdded(const QDateTime& dateAdded) {
    auto oldDate = dateAdded;
    m_dateAdded.swap(oldDate);
    if ( oldDate!=dateAdded) { setDirty(true); }
}
QDateTime TrackInfoObject::getFileModifiedTime() const { return getFileInfo().lastModified(); }
QDateTime TrackInfoObject::getFileCreationTime() const { return getFileInfo().created(); }
double TrackInfoObject::getDuration()  const { return m_dDuration.load(); }

void TrackInfoObject::setDuration(double i) { if (m_dDuration.exchange(i) != i) { setDirty(true); } }
QString TrackInfoObject::getTitle() const { return m_sTitle; }
void TrackInfoObject::setTitle(const QString& s) {
    QString title = s.trimmed();
    auto oldTitle = title;
    m_sTitle.swap(oldTitle);
    if (oldTitle != title) { setDirty(true); }
}
QString TrackInfoObject::getArtist() const { return m_sArtist; }
void TrackInfoObject::setArtist(const QString& s) {
    QString artist = s.trimmed();
    auto oldArtist = artist;
    m_sArtist.swap(oldArtist);
    if (oldArtist != artist) { setDirty(true); }
}
QString TrackInfoObject::getAlbum() const { return m_sAlbum; }
void TrackInfoObject::setAlbum(const QString& s) {
    QString album = s.trimmed();
    auto sAlbum = album;
    m_sAlbum.swap(sAlbum);
    if (sAlbum != album) { setDirty(true); }
}
QString TrackInfoObject::getAlbumArtist()  const { return m_sAlbumArtist; }
void TrackInfoObject::setAlbumArtist(const QString& s) {
    auto st = s.trimmed();
    auto ost = st;
    m_sAlbumArtist.swap(ost);
    if (ost != st) { setDirty(true); }
}
QString TrackInfoObject::getYear()  const { return m_sYear; }
void TrackInfoObject::setYear(const QString& s) {
    QString year = s.trimmed();
    auto ret = year;
    m_sYear.swap(ret);
    if (ret != year) { setDirty(true);}
}
QString TrackInfoObject::getGenre() const { return m_sGenre; }
void TrackInfoObject::setGenre(const QString& s) {
    QString genre = s.trimmed();
    auto ret = genre;
    m_sGenre.swap(ret);
    if (ret != genre) { setDirty(true); }
}
QString TrackInfoObject::getComposer() const { auto ret = m_sComposer;return ret; }

void TrackInfoObject::setComposer(const QString& s) {
    QString composer = s.trimmed();
    auto ret = composer;
    m_sComposer.swap(ret);
    if (ret != composer) { setDirty(true); }
}

QString TrackInfoObject::getGrouping()  const { return m_sGrouping; }

void TrackInfoObject::setGrouping(const QString& s) {
    QString grouping = s.trimmed();
    auto ret = grouping;
    m_sGrouping.swap(ret);
    if (ret != grouping) { setDirty(true); }
}
QString TrackInfoObject::getTrackNumber()  const {
    auto ret = m_sTrackNumber;
    return ret;
}
void TrackInfoObject::setTrackNumber(const QString& s) {
    QString tn = s.trimmed();
    auto ret = tn;
    m_sTrackNumber.swap(ret);
    if (ret != tn) { setDirty(true); }
}
int TrackInfoObject::getTimesPlayed() const { return m_iTimesPlayed.load(); }
void TrackInfoObject::setTimesPlayed(int t) { if (t != m_iTimesPlayed.exchange(t)) { setDirty(true); } }
void TrackInfoObject::incTimesPlayed() { setPlayedAndUpdatePlaycount(true) ;}
bool TrackInfoObject::getPlayed() const { return m_bPlayed.load(); }
void TrackInfoObject::setPlayedAndUpdatePlaycount(bool bPlayed) {
    QMutexLocker lock(&m_qMutex);
    if (bPlayed) {
        m_iTimesPlayed++;
        setDirty(true);
    }
    else if (m_bPlayed && !bPlayed) {
        if (m_iTimesPlayed.fetch_add(-1) <= 0 ) m_iTimesPlayed.store(0);
        setDirty(true);
    }
    m_bPlayed.store(bPlayed);
}
void TrackInfoObject::setPlayed(bool bPlayed) { if ( bPlayed!=m_bPlayed.exchange(bPlayed)){ setDirty(true); } }
QString TrackInfoObject::getComment() const { return m_sComment; }
void TrackInfoObject::setComment(const QString& s) {
    auto ret = s;
    m_sComment.swap(ret);
    if (s !=ret) { setDirty(true); }
}
QString TrackInfoObject::getType() const { return m_sType; }
void TrackInfoObject::setType(const QString& s) {
    auto ret = s;
    m_sType.swap(ret);
    if (s != ret) { setDirty(true); }
}
void TrackInfoObject::setSampleRate(int iSampleRate) { if (m_iSampleRate.exchange(iSampleRate) != iSampleRate) { setDirty(true); } }
int TrackInfoObject::getSampleRate() const { return m_iSampleRate.load(); }
void TrackInfoObject::setChannels(int iChannels) { if (m_iChannels.exchange(iChannels) != iChannels) { setDirty(true); } }
int TrackInfoObject::getChannels() const { return m_iChannels.load(); }
int TrackInfoObject::getLength() const { return getFileInfo().size(); }
int TrackInfoObject::getBitrate() const { return m_iBitrate.load(); }
QString TrackInfoObject::getBitrateStr() const { return QString("%1").arg(getBitrate()); }
void TrackInfoObject::setBitrate(int i) { if (m_iBitrate.exchange(i) != i) { setDirty(true); } }
TrackId TrackInfoObject::getId() const {
    QMutexLocker lock(&m_qMutex);
    return m_id;
}
void TrackInfoObject::setId(TrackId trackId) {
    QMutexLocker lock(&m_qMutex);
    // changing the Id does not make the track dirty because the Id is always
    // generated by the Database itself
    m_id = trackId;
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
    auto ret = url;
    m_sURL.swap(ret);
    if (ret != url) { setDirty(true); }
}
QString TrackInfoObject::getURL() const{ return m_sURL; }
ConstWaveformPointer TrackInfoObject::getWaveform() const { return m_waveform; }
void TrackInfoObject::setWaveform(ConstWaveformPointer pWaveform) {
    auto ret = pWaveform;
    m_waveform.swap(ret);
    if(ret!=pWaveform) emit(waveformUpdated());
}
ConstWaveformPointer TrackInfoObject::getWaveformSummary() const { return m_waveformSummary; }
void TrackInfoObject::setWaveformSummary(ConstWaveformPointer pWaveform) {
    auto ret = pWaveform;
    m_waveformSummary.swap(ret);
    if ( ret != pWaveform) emit(waveformSummaryUpdated());
}
void TrackInfoObject::setAnalyserProgress(double progress) {
    // progress in 0 .. 1000. QAtomicInt so no need for lock.
	  auto oldProgress = m_analyserProgress.exchange(progress);
    if ( progress !=oldProgress) { emit(analyserProgress(progress)); }
}
double TrackInfoObject::getAnalyserProgress() const { return m_analyserProgress.load(); }
void TrackInfoObject::setCuePoint(double cue) { if ( m_dCuePoint.exchange(cue)!=cue){ setDirty(true);} }
double TrackInfoObject::getCuePoint() const { return m_dCuePoint.load(); }
void TrackInfoObject::slotCueUpdated() {
    setDirty(true);
    emit(cuesUpdated());
}
Cue* TrackInfoObject::addCue() {
    //qDebug() << "TrackInfoObject::addCue()";
    QMutexLocker lock(&m_qMutex);
    Cue* cue = new Cue(m_id);
    connect(cue, SIGNAL(updated()), this, SLOT(slotCueUpdated()));
    m_cuePoints.push_back(cue);
    setDirty(true);
    lock.unlock();
    emit(cuesUpdated());
    return cue;
}
void TrackInfoObject::removeCue(Cue* cue) {
    QMutexLocker lock(&m_qMutex);
    disconnect(cue, 0, this, 0);
    // TODO(XXX): Delete the cue point.
    m_cuePoints.removeOne(cue);
    setDirty(true);
    lock.unlock();
    emit(cuesUpdated());
}
const QList<Cue*>& TrackInfoObject::getCuePoints() { return m_cuePoints; }
void TrackInfoObject::setCuePoints(QList<Cue*> cuePoints) {
    //qDebug() << "setCuePoints" << cuePoints.length();
    auto curr_points = m_cuePoints;
    m_cuePoints.swap(curr_points);
    for ( auto cue : curr_points) disconnect(cue, 0, this, 0 );
    for ( auto cue : cuePoints )  connect(cue,SIGNAL(updated()),this,SLOT(slotCueUpdated()));
    setDirty(true);
    emit(cuesUpdated());
}
void TrackInfoObject::setDirty(bool bDirty) {
    // qDebug() << "Track" << m_id << getInfo() << (change? "changed" : "unchanged")
    //          << "set" << (bDirty ? "dirty" : "clean");
    if (m_bDirty.exchange(bDirty)!=bDirty) {
        if (m_bDirty) { emit(dirty(this));}
        else { emit(clean(this)); }
    }
    // Emit a changed signal regardless if this attempted to set us dirty.
    if (bDirty) { emit(changed(this)); }
    //qDebug() << QString("TrackInfoObject %1 %2 set to %3").arg(QString::number(m_id), m_fileInfo.absoluteFilePath(), m_bDirty ? "dirty" : "clean");
}
bool TrackInfoObject::isDirty() { return m_bDirty.load(); }
bool TrackInfoObject::locationChanged() { return m_bLocationChanged.load(); }
int TrackInfoObject::getRating() const { return m_Rating.load(); }
void TrackInfoObject::setRating (int rating) { if (rating != m_Rating.exchange(rating)) { setDirty(true); } }
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
    if (!m_keys.isValid()) { return mixxx::track::io::key::INVALID; }
    return m_keys.getGlobalKey();
}
void TrackInfoObject::setKey(mixxx::track::io::key::ChromaticKey key, mixxx::track::io::key::Source source) {
    QMutexLocker lock(&m_qMutex);
    bool dirty = false;
    if (key == mixxx::track::io::key::INVALID) {
        m_keys = Keys();
        dirty = true;
    } else if (m_keys.getGlobalKey() != key) { m_keys = KeyFactory::makeBasicKeys(key, source);}
    if (dirty) { setDirty(true); }
    // Might be INVALID. We don't care.
    mixxx::track::io::key::ChromaticKey newKey = m_keys.getGlobalKey();
    lock.unlock();
    emit(keyUpdated(KeyUtils::keyToNumericValue(newKey)));
    emit(keysUpdated());
}

void TrackInfoObject::setKeyText(QString key, mixxx::track::io::key::Source source) {
    QMutexLocker lock(&m_qMutex);
    Keys newKeys = KeyFactory::makeBasicKeysFromText(key, source);
    // We treat this as dirtying if it is parsed to a different key or if we
    // fail to parse the key, if the text value is different from the current
    // text value.
    auto dirty = newKeys.getGlobalKey() != m_keys.getGlobalKey() ||
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
    if (key != mixxx::track::io::key::INVALID) {
        return KeyUtils::keyToString(key);
    }

    // Fall back on text global name.
    QString keyText = m_keys.getGlobalKeyText();
    return keyText;
}
void TrackInfoObject::setBpmLock(bool bpmLock) { if (bpmLock != m_bBpmLock.exchange(bpmLock)) { setDirty(true); } }
bool TrackInfoObject::hasBpmLock() const { return m_bBpmLock.load(); }
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
