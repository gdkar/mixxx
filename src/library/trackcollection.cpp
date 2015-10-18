#include <QtSql>
#include <QtDebug>
#include <QMessageBox>
#include "library/trackcollection.h"
#include "library/librarytablemodel.h"
#include "library/schemamanager.h"
#include "trackinfoobject.h"
#include "util/xml.h"
#include "util/assert.h"

// static
const int TrackCollection::kRequiredSchemaVersion = 24;

TrackCollection::TrackCollection(ConfigObject<ConfigValue>* pConfig)
        : m_pConfig(pConfig),
          m_db(QSqlDatabase::addDatabase("QSQLITE")), // defaultConnection
          m_playlistDao(m_db),
          m_crateDao(m_db),
          m_cueDao(m_db),
          m_directoryDao(m_db),
          m_analysisDao(m_db, pConfig),
          m_libraryHashDao(m_db),
          m_trackDao(m_db, m_cueDao, m_playlistDao, m_crateDao,
                     m_analysisDao, m_libraryHashDao, pConfig) {
    qDebug() << "Available QtSQL drivers:" << QSqlDatabase::drivers();
    m_db.setHostName("localhost");
    m_db.setDatabaseName(QDir(pConfig->getSettingsPath()).filePath("mixxxdb.sqlite"));
    m_db.setUserName("mixxx");
    m_db.setPassword("mixxx");
    auto ok = m_db.open();
    qDebug() << "DB status:" << m_db.databaseName() << "=" << ok;
    if (m_db.lastError().isValid()) { qDebug() << "Error loading database:" << m_db.lastError(); }
    // Check for tables and create them if missing
    if (!checkForTables()) { exit(-1); }
}
TrackCollection::~TrackCollection() {
    qDebug() << "~TrackCollection()";
    m_trackDao.finish();
    if (m_db.isOpen()) {
        // There should never be an outstanding transaction when this code is
        // called. If there is, it means we probably aren't committing a
        // transaction somewhere that should be.
        if (m_db.rollback()) {
            qDebug() << "ERROR: There was a transaction in progress on the main database connection while shutting down."
                    << "There is a logic error somewhere.";
        }
        m_db.close();
    } else {
        qDebug() << "ERROR: The main database connection was closed before TrackCollection closed it."
                << "There is a logic error somewhere.";
    }
}
bool TrackCollection::checkForTables() {
    if (!m_db.open()) {
        QMessageBox::critical(0, tr("Cannot open database"),
                            tr("Unable to establish a database connection.\n"
                                "Mixxx requires QT with SQLite support. Please read "
                                "the Qt SQL driver documentation for information on how "
                                "to build it.\n\n"
                                "Click OK to exit."), QMessageBox::Ok);
        return false;
    }
    // The schema XML is baked into the binary via Qt resources.
    QString schemaFilename(":/schema.xml");
    QString okToExit = tr("Click OK to exit.");
    QString upgradeFailed = tr("Cannot upgrade database schema");
    QString upgradeToVersionFailed =
            tr("Unable to upgrade your database schema to version %1")
            .arg(QString::number(kRequiredSchemaVersion));
    QString helpEmail = tr("For help with database issues contact:") + "\n" +
                           "mixxx-devel@lists.sourceforge.net";
    SchemaManager::Result result = SchemaManager::upgradeToSchemaVersion( schemaFilename, m_db, kRequiredSchemaVersion);
    switch (result) {
        case SchemaManager::RESULT_BACKWARDS_INCOMPATIBLE:
            QMessageBox::warning(
                    0, upgradeFailed,
                    upgradeToVersionFailed + "\n" +
                    tr("Your mixxxdb.sqlite file was created by a newer "
                       "version of Mixxx and is incompatible.") +
                    "\n\n" + okToExit,
                    QMessageBox::Ok);
            return false;
        case SchemaManager::RESULT_UPGRADE_FAILED:
            QMessageBox::warning(
                    0, upgradeFailed,
                    upgradeToVersionFailed + "\n" +
                    tr("Your mixxxdb.sqlite file may be corrupt.") + "\n" +
                    tr("Try renaming it and restarting Mixxx.") + "\n" +
                    helpEmail + "\n\n" + okToExit,
                    QMessageBox::Ok);
            return false;
        case SchemaManager::RESULT_SCHEMA_ERROR:
            QMessageBox::warning(
                    0, upgradeFailed,
                    upgradeToVersionFailed + "\n" +
                    tr("The database schema file is invalid.") + "\n" +
                    helpEmail + "\n\n" + okToExit,
                    QMessageBox::Ok);
            return false;
        case SchemaManager::RESULT_OK:
        default:
            break;
    }

    m_trackDao.initialize();
    m_playlistDao.initialize();
    m_crateDao.initialize();
    m_cueDao.initialize();
    m_directoryDao.initialize();
    m_libraryHashDao.initialize();
    return true;
}
QSqlDatabase& TrackCollection::getDatabase() {return m_db; }
CrateDAO& TrackCollection::getCrateDAO() { return m_crateDao; }
TrackDAO& TrackCollection::getTrackDAO() { return m_trackDao; }
PlaylistDAO& TrackCollection::getPlaylistDAO() { return m_playlistDao; }
DirectoryDAO& TrackCollection::getDirectoryDAO() { return m_directoryDao; }
QSharedPointer<BaseTrackCache> TrackCollection::getTrackSource() { return m_defaultTrackSource; }
void TrackCollection::setTrackSource(QSharedPointer<BaseTrackCache> trackSource) {
    DEBUG_ASSERT_AND_HANDLE(m_defaultTrackSource.isNull()) { return; }
    m_defaultTrackSource = trackSource;
}

