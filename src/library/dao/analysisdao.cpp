#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>
#include <QTime>
#include <QtDebug>

#include "waveform/waveform.h"
#include "library/dao/analysisdao.h"
#include "library/queryutil.h"

const QString AnalysisDao::s_analysisTableName = "track_analysis";

// For a track that takes 1.2MB to store the big waveform, the default
// compression level (-1) takes the size down to about 600KB. The difference
// between the default and 9 (the max) was only about 1-2KB for a lot of extra
// CPU time so I think we should stick with the default. rryan 4/3/2012
const int kCompressionLevel = -1;

AnalysisDao::AnalysisDao(QSqlDatabase& database, ConfigObject<ConfigValue>* pConfig)
        : m_pConfig(pConfig),
          m_db(database)
{
    QDir storagePath = getAnalysisStoragePath();
    if (!QDir().mkpath(storagePath.absolutePath())) 
        qDebug() << "WARNING: Could not create analysis storage path. Mixxx will be unable to store analyses.";
}
AnalysisDao::~AnalysisDao() = default;
void AnalysisDao::initialize()
{
}
void AnalysisDao::setDatabase(QSqlDatabase& database)
{
    m_db = database;
}
QList<AnalysisDao::AnalysisInfo> AnalysisDao::getAnalysesForTrack(TrackId trackId)
{
    if (!m_db.isOpen() || !trackId.isValid()) return QList<AnalysisInfo>();
    QSqlQuery query(m_db);
    query.prepare(QString("SELECT id, type, description, version, data_checksum FROM %1 "
                          "WHERE track_id=:trackId").arg(s_analysisTableName));
    query.bindValue(":trackId", trackId.toVariant());
    return loadAnalysesFromQuery(trackId, &query);
}
QList<AnalysisDao::AnalysisInfo> AnalysisDao::getAnalysesForTrackByType(TrackId trackId, AnalysisType type)
{
    if (!m_db.isOpen() || !trackId.isValid())return QList<AnalysisInfo>();
    QSqlQuery query(m_db);
    query.prepare(QString(
        "SELECT id, type, description, version, data_checksum FROM %1 "
        "WHERE track_id=:trackId AND type=:type").arg(s_analysisTableName));
    query.bindValue(":trackId", trackId.toVariant());
    query.bindValue(":type", type);
    return loadAnalysesFromQuery(trackId, &query);
}
QList<AnalysisDao::AnalysisInfo> AnalysisDao::loadAnalysesFromQuery(TrackId trackId, QSqlQuery* query)
{
    QList<AnalysisDao::AnalysisInfo> analyses;
    QTime time;
    time.start();
    if (!query->exec())
    {
        LOG_FAILED_QUERY(*query) << "couldn't get analyses for track" << trackId;
        return analyses;
    }
    auto bytes = 0;
    auto queryRecord = query->record();
    auto idColumn = queryRecord.indexOf("id");
    auto typeColumn = queryRecord.indexOf("type");
    auto descriptionColumn = queryRecord.indexOf("description");
    auto versionColumn = queryRecord.indexOf("version");
    auto dataChecksumColumn = queryRecord.indexOf("data_checksum");
    while (query->next())
    {
        AnalysisDao::AnalysisInfo info;
        info.analysisId = query->value(idColumn).toInt();
        info.trackId = trackId;
        info.type = static_cast<AnalysisType>(query->value(typeColumn).toInt());
        info.description = query->value(descriptionColumn).toString();
        info.version = query->value(versionColumn).toString();
        auto checksum = query->value(dataChecksumColumn).toInt();
        auto dataPath = getAnalysisStoragePath().absoluteFilePath(QString::number(info.analysisId));
        auto compressedData = loadDataFromFile(dataPath);
        auto file_checksum = qChecksum(compressedData.constData(),compressedData.length());
        if (checksum != file_checksum)
        {
            qDebug() << "WARNING: Corrupt analysis loaded from" << dataPath
                     << "length" << compressedData.length();
            continue;
        }
        info.data = qUncompress(compressedData);
        bytes += info.data.length();
        analyses.append(info);
    }
    qDebug() << "AnalysisDAO fetched" << analyses.size() << "analyses,"
             << bytes << "bytes for track"
             << trackId << "in" << time.elapsed() << "ms";
    return analyses;
}
bool AnalysisDao::saveAnalysis(AnalysisDao::AnalysisInfo* info)
{
    if (!m_db.isOpen() || !info) return false;
    if (!info->trackId.isValid())
    {
        qDebug() << "Can't save analysis since trackId is invalid.";
        return false;
    }
    QTime time;
    time.start();
    auto compressedData = qCompress(info->data, kCompressionLevel);
    auto checksum = qChecksum(compressedData.constData(),compressedData.length());
    QSqlQuery query(m_db);
    if (info->analysisId == -1)
    {
        query.prepare(QString(
            "INSERT INTO %1 (track_id, type, description, version, data_checksum) "
            "VALUES (:trackId,:type,:description,:version,:data_checksum)")
                      .arg(s_analysisTableName));

        QByteArray waveformBytes;
        query.bindValue(":trackId", info->trackId.toVariant());
        query.bindValue(":type", info->type);
        query.bindValue(":description", info->description);
        query.bindValue(":version", info->version);
        query.bindValue(":data_checksum", checksum);
        if (!query.exec())
        {
            LOG_FAILED_QUERY(query) << "couldn't save new analysis";
            return false;
        }
        info->analysisId = query.lastInsertId().toInt();
    } else {
        query.prepare(QString(
            "UPDATE %1 SET "
            "track_id = :trackId,"
            "type = :type,"
            "description = :description,"
            "version = :version,"
            "data_checksum = :data_checksum "
            "WHERE id = :analysisId").arg(s_analysisTableName));

        query.bindValue(":analysisId", info->analysisId);
        query.bindValue(":trackId", info->trackId.toVariant());
        query.bindValue(":type", info->type);
        query.bindValue(":description", info->description);
        query.bindValue(":version", info->version);
        query.bindValue(":data_checksum", checksum);
        if (!query.exec())
        {
            LOG_FAILED_QUERY(query) << "couldn't update existing analysis";
            return false;
        }
    }
    auto dataPath = getAnalysisStoragePath().absoluteFilePath(QString::number(info->analysisId));
    if (!saveDataToFile(dataPath, compressedData))
    {
        qDebug() << "WARNING: Couldn't save analysis data to file" << dataPath;
        return false;
    }
    qDebug() << "AnalysisDAO saved analysis" << info->analysisId
             << QString("%1 (%2 compressed)").arg(QString::number(info->data.length()),
                                                  QString::number(compressedData.length()))
             << "bytes for track"
             << info->trackId << "in" << time.elapsed() << "ms";
    return true;
}
bool AnalysisDao::deleteAnalysis(const int analysisId)
{
    if (analysisId == -1)return false;
    QSqlQuery query(m_db);
    query.prepare(QString("DELETE FROM %1 WHERE id = :id").arg(s_analysisTableName));
    query.bindValue(":id", analysisId);
    if (!query.exec())
    {
        LOG_FAILED_QUERY(query) << "couldn't delete analysis";
        return false;
    }
    auto dataPath = getAnalysisStoragePath().absoluteFilePath(QString::number(analysisId));
    deleteFile(dataPath);
    return true;
}
void AnalysisDao::deleteAnalyses(const QList<TrackId>& trackIds)
{
    QStringList idList;
    for (auto trackId: trackIds)
    {
        idList << trackId.toString();
    }
    QSqlQuery query(m_db);
    query.prepare(QString("SELECT track_analysis.id FROM track_analysis WHERE "
                          "track_id in (%1)").arg(idList.join(",")));
    if (!query.exec()) LOG_FAILED_QUERY(query) << "couldn't delete analysis";
    auto idColumn = query.record().indexOf("id");
    while (query.next()) {
        auto id = query.value(idColumn).toInt();
        auto dataPath = getAnalysisStoragePath().absoluteFilePath(QString::number(id));
        qDebug() << dataPath;
        deleteFile(dataPath);
    }
    query.prepare(QString("DELETE FROM track_analysis WHERE track_id in (%1)").arg(idList.join(",")));
    if (!query.exec()) LOG_FAILED_QUERY(query) << "couldn't delete analysis";
}
bool AnalysisDao::deleteAnalysesForTrack(TrackId trackId)
{
    if (!trackId.isValid()) return false;
    QSqlQuery query(m_db);
    query.prepare(QString("SELECT id FROM %1 where track_id = :track_id").arg(s_analysisTableName));
    query.bindValue(":track_id", trackId.toVariant());
    if (!query.exec())
    {
        LOG_FAILED_QUERY(query) << "couldn't delete analyses for track" << trackId;
        return false;
    }
    QList<int> analysesToDelete;
    auto idColumn = query.record().indexOf("id");
    while (query.next())
    {
        analysesToDelete.append(query.value(idColumn).toInt());
    }
    for(auto analysisId: analysesToDelete) deleteAnalysis(analysisId);
    return true;
}
QDir AnalysisDao::getAnalysisStoragePath() const
{
    auto settingsPath = m_pConfig->getSettingsPath();
    auto dir = QDir(settingsPath.append("/analysis/"));
    return dir.absolutePath().append("/");
}
QByteArray AnalysisDao::loadDataFromFile(QString filename) const
{
    QFile file(filename);
    if (!file.exists())return QByteArray();
    if (!file.open(QIODevice::ReadOnly))return QByteArray();
    return file.readAll();
}
bool AnalysisDao::deleteFile(QString fileName) const
{
    QFile file(fileName);
    return file.remove();
}
bool AnalysisDao::saveDataToFile(QString fileName, QByteArray data) const
{
    QFile file(fileName);
    // If the file exists, do the right thing. Write to a temp file, unlink the
    // existing file, and then move the temp file to the real file's name.
    if (file.exists())
    {
        auto tempFileName = fileName + ".tmp";
        QFile tempFile(tempFileName);
        if (!tempFile.open(QIODevice::WriteOnly))return false;
        auto bytesWritten = tempFile.write(data);
        if (bytesWritten == -1 || bytesWritten != data.length()) return false;
        tempFile.close();
        if (!file.remove()) return false;
        if (!tempFile.rename(fileName)) return false;
        return true;
    }
    // If the file doesn't exist, just create a new file and write the data.
    if (!file.open(QIODevice::WriteOnly)) return false;
    auto bytesWritten = file.write(data);
    if (bytesWritten == -1 || bytesWritten != data.length()) return false;
    file.close();
    return true;
}
void AnalysisDao::saveTrackAnalyses(TrackInfoObject* pTrack)
{
    if (!pTrack) return;
    auto pWaveform = pTrack->getWaveform();
    auto pWaveSummary = pTrack->getWaveformSummary();
    // Don't try to save invalid or non-dirty waveforms.
    if (!pWaveform || !pWaveform->size() || !pWaveform->isDirty() ||
        !pWaveSummary || !pWaveSummary->size() || !pWaveSummary->isDirty())
    {
        return;
    }
    auto trackId = TrackId(pTrack->getId());
    AnalysisDao::AnalysisInfo analysis;
    analysis.trackId = trackId;
    if (pWaveform->getId() != -1) analysis.analysisId = pWaveform->getId();
    analysis.type = AnalysisDao::TYPE_WAVEFORM;
    analysis.description = pWaveform->getDescription();
    analysis.version = pWaveform->getVersion();
    analysis.data = pWaveform->toByteArray();
    auto success = saveAnalysis(&analysis);
    if (success) pWaveform->setDirty(false);
    qDebug() << (success ? "Saved" : "Failed to save") << "waveform analysis for trackId" << trackId << "analysisId" << analysis.analysisId;
    // Clear analysisId since we are re-using the AnalysisInfo
    analysis.analysisId = -1;
    analysis.type = AnalysisDao::TYPE_WAVESUMMARY;
    analysis.description = pWaveSummary->getDescription();
    analysis.version = pWaveSummary->getVersion();
    analysis.data = pWaveSummary->toByteArray();
    success = saveAnalysis(&analysis);
    if (success) pWaveSummary->setDirty(false);
    qDebug() << (success ? "Saved" : "Failed to save") << "waveform summary analysis for trackId" << trackId << "analysisId" << analysis.analysisId;
}
