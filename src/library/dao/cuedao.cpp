// cuedao.cpp
// Created 10/26/2009 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>
#include <QtSql>
#include <QVariant>

#include "library/dao/cuedao.h"
#include "library/dao/cue.h"
#include "trackinfoobject.h"
#include "library/queryutil.h"
#include "util/assert.h"

CueDAO::CueDAO(QSqlDatabase& database)
        : m_database(database) {
}
CueDAO::~CueDAO() = default;
void CueDAO::initialize()
{
    qDebug() << "CueDAO::initialize" << QThread::currentThread() << m_database.connectionName();
}
int CueDAO::cueCount()
{
    qDebug() << "CueDAO::cueCount" << QThread::currentThread() << m_database.connectionName();
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM " CUE_TABLE);
    if (query.exec())
    {
        if (query.next()) return query.value(0).toInt();
    } else LOG_FAILED_QUERY(query);
    //query.finish();
    return 0;
}
int CueDAO::numCuesForTrack(TrackId trackId)
{
    qDebug() << "CueDAO::numCuesForTrack" << QThread::currentThread() << m_database.connectionName();
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM " CUE_TABLE " WHERE track_id = :id");
    query.bindValue(":id", trackId.toVariant());
    if (query.exec()) {
        if (query.next()) return query.value(0).toInt();
    } else LOG_FAILED_QUERY(query);
    return 0;
}
Cue* CueDAO::cueFromRow(const QSqlQuery& query) const
{
    QSqlRecord record = query.record();
    auto id = record.value(record.indexOf("id")).toInt();
    auto trackId = TrackId(record.value(record.indexOf("track_id")));
    auto type = record.value(record.indexOf("type")).toInt();
    auto position = record.value(record.indexOf("position")).toInt();
    auto length = record.value(record.indexOf("length")).toInt();
    auto hotcue = record.value(record.indexOf("hotcue")).toInt();
    auto label = record.value(record.indexOf("label")).toString();
    auto cue = new Cue(id, trackId, (Cue::CueType)type,position, length, hotcue, label);
    m_cues[id] = cue;
    return cue;
}
QList<Cue*> CueDAO::getCuesForTrack(TrackId trackId) const
{
    //qDebug() << "CueDAO::getCuesForTrack" << QThread::currentThread() << m_database.connectionName();
    QList<Cue*> cues;
    // A hash from hotcue index to cue id and cue*, used to detect if more
    // than one cue has been assigned to a single hotcue id.
    QMap<int, QPair<int, Cue*> > dupe_hotcues;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM " CUE_TABLE " WHERE track_id = :id");
    query.bindValue(":id", trackId.toVariant());
    if (query.exec())
    {
        auto idColumn = query.record().indexOf("id");
        auto hotcueIdColumn = query.record().indexOf("hotcue");
        while (query.next())
        {
            auto cue = static_cast<Cue*>(nullptr);
            auto cueId = query.value(idColumn).toInt();
            if (m_cues.contains(cueId)) cue = m_cues[cueId];
            if (!cue ) cue = cueFromRow(query);
            auto hotcueId = query.value(hotcueIdColumn).toInt();
            if (hotcueId != -1)
            {
                if (dupe_hotcues.contains(hotcueId))
                {
                    m_cues.remove(dupe_hotcues[hotcueId].first);
                    cues.removeOne(dupe_hotcues[hotcueId].second);
                }
                dupe_hotcues[hotcueId] = qMakePair(cueId, cue);
            }
            if (cue )  cues.push_back(cue);
        }
    } else LOG_FAILED_QUERY(query);
    return cues;
}

bool CueDAO::deleteCuesForTrack(TrackId trackId) {
    qDebug() << "CueDAO::deleteCuesForTrack" << QThread::currentThread() << m_database.connectionName();
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM " CUE_TABLE " WHERE track_id = :track_id");
    query.bindValue(":track_id", trackId.toVariant());
    if (query.exec()) return true;
    else LOG_FAILED_QUERY(query);
    return false;
}
bool CueDAO::deleteCuesForTracks(const QList<TrackId>& trackIds) {
    qDebug() << "CueDAO::deleteCuesForTracks" << QThread::currentThread() << m_database.connectionName();
    QStringList idList;
    for (auto trackId: trackIds) idList << trackId.toString();
    QSqlQuery query(m_database);
    query.prepare(QString("DELETE FROM " CUE_TABLE " WHERE track_id in (%1)").arg(idList.join(",")));
    if (query.exec()) return true;
    else LOG_FAILED_QUERY(query);
    return false;
}

bool CueDAO::saveCue(Cue* cue) {
    //qDebug() << "CueDAO::saveCue" << QThread::currentThread() << m_database.connectionName();
    DEBUG_ASSERT_AND_HANDLE(cue) {return false;}
    if (cue->getId() == -1)
    {
        // New cue
        QSqlQuery query(m_database);
        query.prepare("INSERT INTO " CUE_TABLE " (track_id, type, position, length, hotcue, label) VALUES (:track_id, :type, :position, :length, :hotcue, :label)");
        query.bindValue(":track_id", cue->getTrackId().toVariant());
        query.bindValue(":type", cue->getType());
        query.bindValue(":position", cue->getPosition());
        query.bindValue(":length", cue->getLength());
        query.bindValue(":hotcue", cue->getHotCue());
        query.bindValue(":label", cue->getLabel());
        if (query.exec())
        {
            auto id = query.lastInsertId().toInt();
            cue->setId(id);
            cue->setDirty(false);
            return true;
        }
        qDebug() << query.executedQuery() << query.lastError();
    }
    else
    {
        // Update cue
        QSqlQuery query(m_database);
        query.prepare("UPDATE " CUE_TABLE " SET "
                        "track_id = :track_id,"
                        "type = :type,"
                        "position = :position,"
                        "length = :length,"
                        "hotcue = :hotcue,"
                        "label = :label"
                        " WHERE id = :id");
        query.bindValue(":id", cue->getId());
        query.bindValue(":track_id", cue->getTrackId().toVariant());
        query.bindValue(":type", cue->getType());
        query.bindValue(":position", cue->getPosition());
        query.bindValue(":length", cue->getLength());
        query.bindValue(":hotcue", cue->getHotCue());
        query.bindValue(":label", cue->getLabel());
        if (query.exec())
        {
            cue->setDirty(false);
            return true;
        } else  LOG_FAILED_QUERY(query);
    }
    return false;
}
bool CueDAO::deleteCue(Cue* cue)
{
    //qDebug() << "CueDAO::deleteCue" << QThread::currentThread() << m_database.connectionName();
    if (cue->getId() != -1)
    {
        QSqlQuery query(m_database);
        query.prepare("DELETE FROM " CUE_TABLE " WHERE id = :id");
        query.bindValue(":id", cue->getId());
        if (query.exec())  return true;
        else LOG_FAILED_QUERY(query);
    } else  return true;
    return false;
}

void CueDAO::saveTrackCues(TrackId trackId, TrackInfoObject* pTrack) {
    //qDebug() << "CueDAO::saveTrackCues" << QThread::currentThread() << m_database.connectionName();
    // TODO(XXX) transaction, but people who are already in a transaction call
    // this.
    QTime time;
    auto cueList = pTrack->getCuePoints();
    // qDebug() << "CueDAO::saveTrackCues old size:" << oldCueList.size()
    //          << "new size:" << cueList.size();

    auto list = QString{""};
    time.start();
    // For each id still in the TIO, save or delete it.
    QListIterator<Cue*> cueIt(cueList);
    while (cueIt.hasNext())
    {
        auto cue = cueIt.next();
        auto cueId = cue->getId();
        auto newCue = cueId == -1;
        if (newCue) cue->setTrackId(trackId);
        else list.append(QString("%1,").arg(cueId));
        // Update or save cue
        if (cue->isDirty()) {
            saveCue(cue);
            // Since this cue didn't have an id until now, add it to the list of
            // cues not to delete.
            if (newCue)list.append(QString("%1,").arg(cue->getId()));
        }
    }
    //qDebug() << "Saving cues took " << time.elapsed() << "ms";
    time.start();
    // Strip the last ,
    if (list.count() > 0) list.truncate(list.count()-1);
    // Delete cues that are no longer on the track.
    QSqlQuery query(m_database);
    query.prepare(QString("DELETE FROM cues where track_id=:track_id and not id in (%1)").arg(list));
    query.bindValue(":track_id", trackId.toVariant());
    if (!query.exec()) LOG_FAILED_QUERY(query) << "Delete cues failed.";
}
