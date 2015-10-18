/*
 * browsethread.cpp         (C) 2011 Tobias Rafreider
 */

#include <QtDebug>
#include <QStringList>
#include <QDateTime>
#include <QDirIterator>

#include "library/browse/browsetablemodel.h"
#include "soundsourceproxy.h"
#include "metadata/trackmetadata.h"
#include "util/time.h"
#include "util/trace.h"


QWeakPointer<BrowseThread> BrowseThread::m_weakInstanceRef;
static QMutex s_Mutex;

/*
 * This class is a singleton and represents a thread
 * that is used to read ID3 metadata
 * from a particular folder.
 *
 * The BrowseTableModel uses this class.
 * Note: Don't call getInstance() from places
 * other than the GUI thread. BrowseThreads emit
 * signals to BrowseModel objects. It does not
 * make sense to use this class in non-GUI threads
 */
BrowseThread::BrowseThread(QObject *parent)
        : QThread(parent) {
    m_bStopThread.store(false);
    m_model_observer = nullptr;
    //start Thread
    start(QThread::LowPriority);
}
BrowseThread::~BrowseThread() {
    qDebug() << "Wait to finish browser background thread";
    m_bStopThread.store(true);
    //wake up thread since it might wait for user input
    m_locationUpdated.wakeAll();
    //Wait until thread terminated
    //terminate();
    wait();
    qDebug() << "Browser background thread terminated!";
}

// static
BrowseThreadPointer BrowseThread::getInstanceRef() {
    BrowseThreadPointer strong = m_weakInstanceRef.toStrongRef();
    if (!strong) {
        s_Mutex.lock();
        strong = m_weakInstanceRef.toStrongRef();
        if (!strong) {
            strong = BrowseThreadPointer(new BrowseThread());
            m_weakInstanceRef = strong.toWeakRef();
        }
        s_Mutex.unlock();
    }
    return strong;
}

void BrowseThread::executePopulation(const MDir& path, BrowseTableModel* client) {
    m_path_mutex.lock();
    m_path = path;
    m_model_observer = client;
    m_path_mutex.unlock();
    m_locationUpdated.wakeAll();
}
void BrowseThread::run() {
    QThread::currentThread()->setObjectName("BrowseThread");
    m_mutex.lock();
    while (!m_bStopThread.load()) {
        //Wait until the user has selected a folder
        m_locationUpdated.wait(&m_mutex);
        Trace trace("BrowseThread");
        //Terminate thread if Mixxx closes
        if(m_bStopThread.load()) {break;}
        // Populate the model
        populateModel();
    }
    m_mutex.unlock();
}
namespace {
class YearItem: public QStandardItem {
public:
    explicit YearItem(QString year): QStandardItem(year) { }
    QVariant data(int role) const {
        switch (role) {
        case Qt::DisplayRole:
        {
            const QString year(QStandardItem::data(role).toString());
            return Mixxx::TrackMetadata::formatCalendarYear(year);
        }
        default: return QStandardItem::data(role);
        }
    }
};
}
void BrowseThread::populateModel()
{
    m_path_mutex.lock();
    MDir thisPath = m_path;
    BrowseTableModel* thisModelObserver = m_model_observer;
    m_path_mutex.unlock();
    // Refresh the name filters in case we loaded new SoundSource plugins.
    QStringList nameFilters(SoundSourceProxy::getSupportedFileNamePatterns());
    QDirIterator fileIt(thisPath.dir().absolutePath(), nameFilters, QDir::Files | QDir::NoDotAndDotDot);
    // remove all rows
    // This is a blocking operation
    // see signal/slot connection in BrowseTableModel
    emit(clearModel(thisModelObserver));
    QList< QList<QStandardItem*> > rows;
    auto row = 0;
    // Iterate over the files
    while (fileIt.hasNext())
    {
        // If a user quickly jumps through the folders
        // the current task becomes "dirty"
        m_path_mutex.lock();
        MDir newPath = m_path;
        m_path_mutex.unlock();
        if (thisPath.dir() != newPath.dir()) {
            qDebug() << "Abort populateModel()";
            return populateModel();
        }
        auto filepath = fileIt.next();
        TrackInfoObject tio(filepath, thisPath.token());
        QList<QStandardItem*> row_data;
        auto item = new QStandardItem("0");
        item->setData("0", Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::preview), item);
        item = new QStandardItem(tio.getFilename());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::filename), item);
        item = new QStandardItem(tio.getArtist());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::artist), item);
        item = new QStandardItem(tio.getTitle());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::title), item);
        item = new QStandardItem(tio.getAlbum());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::album), item);
        item = new QStandardItem(tio.getAlbumArtist());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::album_artist), item);
        item = new QStandardItem(tio.getTrackNumber());
        item->setToolTip(item->text());
        item->setData(item->text().toInt(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::track_number), item);
        auto year = tio.getYear();
        item = new YearItem(year);
        item->setToolTip(year);
        // The year column is sorted according to the numeric calendar year
        item->setData(Mixxx::TrackMetadata::parseCalendarYear(year), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::year), item);
        item = new QStandardItem(tio.getGenre());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::genre), item);
        item = new QStandardItem(tio.getComposer());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::composer), item);
        item = new QStandardItem(tio.getGrouping());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::grouping), item);
        item = new QStandardItem(tio.getComment());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::comment), item);
        auto duration = Time::formatSeconds(qVariantValue<int>( tio.getDuration()), false);
        item = new QStandardItem(duration);
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::duration), item);
        item = new QStandardItem(tio.getBpmStr());
        item->setToolTip(item->text());
        item->setData(tio.getBpm(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::bpm), item);
        item = new QStandardItem(tio.getKeyText());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::key), item);
        item = new QStandardItem(tio.getType());
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::type), item);
        item = new QStandardItem(tio.getBitrateStr());
        item->setToolTip(item->text());
        item->setData(tio.getBitrate(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::bitrate), item);
        item = new QStandardItem(filepath);
        item->setToolTip(item->text());
        item->setData(item->text(), Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::location), item);
        auto modifiedTime = tio.getFileModifiedTime().toLocalTime();
        item = new QStandardItem(modifiedTime.toString(Qt::DefaultLocaleShortDate));
        item->setToolTip(item->text());
        item->setData(modifiedTime, Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::file_modification_time), item);
        auto creationTime = tio.getFileCreationTime().toLocalTime();
        item = new QStandardItem(creationTime.toString(Qt::DefaultLocaleShortDate));
        item->setToolTip(item->text());
        item->setData(creationTime, Qt::UserRole);
        row_data.insert(static_cast<int>(BrowseTableModel::Column::file_creation_time), item);
        rows.append(row_data);
        ++row;
        // If 10 tracks have been analyzed, send it to GUI
        // Will limit GUI freezing
        if (row % 16 == 0)
        {
            // this is a blocking operation
            emit(rowsAppended(rows, thisModelObserver));
            qDebug() << "Append " << rows.count() << " from " << filepath;
            rows.clear();
        }
        // Sleep additionally for 10ms which prevents us from GUI freezes
        msleep(20);
    }
    emit(rowsAppended(rows, thisModelObserver));
    qDebug() << "Append last " << rows.count();
}
