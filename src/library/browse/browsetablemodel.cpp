#include <QtSql>
#include <QStringList>
#include <QtConcurrentRun>
#include <QMetaType>
#include <QMessageBox>
#include <QUrl>

#include "playermanager.h"
#include "library/browse/browsetablemodel.h"
#include "library/browse/browsethread.h"
#include "library/previewbuttondelegate.h"
#include "playerinfo.h"
#include "controlobject.h"
#include "library/dao/trackdao.h"
#include "metadata/trackmetadatataglib.h"
#include "util/dnd.h"

BrowseTableModel::BrowseTableModel(QObject* parent,
                                   TrackCollection* pTrackCollection,
                                   RecordingManager* pRecordingManager)
        : TrackModel(pTrackCollection->getDatabase(),
                     "mixxx.db.model.browse"),
          QStandardItemModel(parent),
          m_pTrackCollection(pTrackCollection),
          m_pRecordingManager(pRecordingManager),
          m_previewDeckGroup(PlayerManager::groupForPreviewDeck(0)) {
    QStringList header_data;
    auto colEnum = QMetaEnum::fromType<Column>();

    for(auto i = 0; i < colEnum.keyCount(); i++)
    {
      header_data.insert(colEnum.value(i),tr(colEnum.key(i)));
    }
    for(auto i = 0; i < colEnum.keyCount(); i++)
    {
      addSearchColumn(colEnum.value(i));
    }
    setDefaultSort(static_cast<int>(Column::filename), Qt::AscendingOrder);
    setHorizontalHeaderLabels(header_data);
    // register the QList<T> as a metatype since we use QueuedConnection below
    qRegisterMetaType< QList< QList<QStandardItem*> > >( "QList< QList<QStandardItem*> >");
    qRegisterMetaType<BrowseTableModel*>("BrowseTableModel*");
    m_pBrowseThread = BrowseThread::getInstanceRef();
    connect(m_pBrowseThread.data(),&BrowseThread::clearModel,this,&BrowseTableModel::slotClear,Qt::QueuedConnection);
    connect(m_pBrowseThread.data(),
            &BrowseThread::rowsAppended,
            this,
            &BrowseTableModel::slotInsert,
            Qt::QueuedConnection
        );
    connect(&PlayerInfo::instance(),&PlayerInfo::trackLoaded,this,&BrowseTableModel::trackLoaded);
    trackLoaded(m_previewDeckGroup, PlayerInfo::instance().getTrackInfo(m_previewDeckGroup));
}
BrowseTableModel::~BrowseTableModel() {}
const QList<int>& BrowseTableModel::searchColumns() const
{ 
  return m_searchColumns;
}
void BrowseTableModel::addSearchColumn(int index)
{ 
  m_searchColumns.push_back(index);
}
void BrowseTableModel::setPath(const MDir& path)
{
    m_current_directory = path;
    m_pBrowseThread->executePopulation(m_current_directory, this);
}
TrackPointer BrowseTableModel::getTrack(const QModelIndex& index) const
{
    auto track_location = getTrackLocation(index);
    if (m_pRecordingManager->getRecordingLocation() == track_location)
    {
        QMessageBox::critical(
            0, tr("Mixxx Library"),
            tr("Could not load the following file because"
               " it is in use by Mixxx or another application.")
            + "\n" +track_location);
        return TrackPointer();
    }
    return m_pTrackCollection->getTrackDAO() .getOrAddTrack(track_location, true, nullptr);
}
QString BrowseTableModel::getTrackLocation(const QModelIndex& _index) const
{
    auto row = _index.row();
    auto index2 = index(row, static_cast<int>(Column::location));
    return data(index2).toString();
}
TrackId BrowseTableModel::getTrackId(const QModelIndex& /*index*/) const
{ 
  return TrackId();
}
const QLinkedList<int> BrowseTableModel::getTrackRows(TrackId /*trackId*/) const
{ 
  return QLinkedList<int>();
}
void BrowseTableModel::search(const QString& /*searchText*/, const QString& /*extraFilter*/)
{
}
const QString BrowseTableModel::currentSearch() const
{ 
  return QString("");
}
bool BrowseTableModel::isColumnInternal(int)
{ 
  return false;
}
bool BrowseTableModel::isColumnHiddenByDefault(int column)
{
    auto col = static_cast<Column>(column);
    return (col == Column::composer ||
            col == Column::track_number||
            col == Column::year ||
            col == Column::grouping||
            col == Column::location ||
            col == Column::file_modification_time ||
            col == Column::file_creation_time
            );
}
void BrowseTableModel::moveTrack(const QModelIndex&, const QModelIndex&)
{
}
void BrowseTableModel::removeTrack(const QModelIndex& index)
{
    if (!index.isValid()) return;
    QStringList trackLocations;
    trackLocations.append(getTrackLocation(index));
    removeTracks(trackLocations);
}
void BrowseTableModel::removeTracks(const QModelIndexList& indices)
{
    QStringList trackLocations;
    for(auto index: indices)
    {
        if (!index.isValid()) continue;
        trackLocations.append(getTrackLocation(index));
    }
    removeTracks(trackLocations);
}
void BrowseTableModel::removeTracks(QStringList trackLocations)
{
    if (!trackLocations.size()) return;
    // Ask user if s/he is sure
    if (QMessageBox::question(
        NULL, tr("Mixxx Library"),
        tr("Warning: This will permanently delete the following files:")
        + "\n" + trackLocations.join("\n") + "\n" +
        tr("Are you sure you want to delete these files from your computer?"),
        QMessageBox::Yes, QMessageBox::Abort) == QMessageBox::Abort) {
        return;
    }
    QList<TrackId> deleted_ids;
    auto any_deleted = false;
    auto& track_dao = m_pTrackCollection->getTrackDAO();
    for(auto track_location: trackLocations)
    {
        // If track is in use or deletion fails, show an error message.
        if (isTrackInUse(track_location) || !QFile::remove(track_location))
        {
            QMessageBox::critical(
                0, tr("Mixxx Library"),
                tr("Could not delete the following file because"
                   " it is in use by Mixxx or another application:") + "\n" +track_location);
            continue;
        }
        qDebug() << "BrowseFeature: User deleted track " << track_location;
        any_deleted = true;
        deleted_ids.append(track_dao.getTrackId(track_location));
    }
    // If the tracks are contained in the Mixxx library, delete them
    if (!deleted_ids.isEmpty())
    {
        qDebug() << "BrowseFeature: Purge affected track from database";
        track_dao.purgeTracks(deleted_ids);
    }
    // Repopulate model if any tracks were actually deleted
    if (any_deleted)
    {
        m_pBrowseThread->executePopulation(m_current_directory, this);
    }
}
bool BrowseTableModel::addTrack(const QModelIndex& /*index*/, QString /*location*/)
{ 
  return false;
}
QMimeData* BrowseTableModel::mimeData(const QModelIndexList &indexes) const
{
    auto mimeData = new QMimeData();
    QList<QUrl> urls;
    // Ok, so the list of indexes we're given contains separates indexes for
    // each column, so even if only one row is selected, we'll have like 7
    // indexes.  We need to only count each row once:
    QList<int> rows;
    for(auto _index: indexes)
    {
        if (_index.isValid())
        {
            if (!rows.contains(_index.row()))
            {
                rows.push_back(_index.row());
                auto  url = DragAndDropHelper::urlFromLocation(getTrackLocation(_index));
                if (!url.isValid())
                {
                    qDebug() << "ERROR invalid url" << url;
                    continue;
                }
                urls.append(url);
            }
        }
    }
    mimeData->setUrls(urls);
    return mimeData;
}
void BrowseTableModel::slotClear(BrowseTableModel* caller_object)
{ 
  if (caller_object == this) removeRows(0, rowCount());
}
void BrowseTableModel::slotInsert(const QList< QList<QStandardItem*> >& rows, BrowseTableModel* caller_object)
{
    // There exists more than one BrowseTableModel in Mixxx We only want to
    // receive items here, this object has 'ordered' by the BrowserThread
    // (singleton)
    if (caller_object == this)
    {
        //qDebug() << "BrowseTableModel::slotInsert";
        for(auto row : rows ) appendRow(row);
    }
}
TrackModel::CapabilitiesFlags BrowseTableModel::getCapabilities() const
{
    // See src/library/trackmodel.h for the list of TRACKMODELCAPS
    return    TRACKMODELCAPS_NONE
            | TRACKMODELCAPS_ADDTOPLAYLIST
            | TRACKMODELCAPS_ADDTOCRATE
            | TRACKMODELCAPS_ADDTOAUTODJ
            | TRACKMODELCAPS_LOADTODECK
            | TRACKMODELCAPS_LOADTOPREVIEWDECK
            | TRACKMODELCAPS_LOADTOSAMPLER;
}
Qt::ItemFlags BrowseTableModel::flags(const QModelIndex &index) const
{
    auto defaultFlags = QAbstractItemModel::flags(index);
    // Enable dragging songs from this data model to elsewhere (like the
    // waveform widget to load a track into a Player).
    defaultFlags |= Qt::ItemIsDragEnabled;
    auto track_location = getTrackLocation(index);
    auto col = static_cast<Column>(index.column());
    if (isTrackInUse(track_location) ||
            col == Column::filename ||
            col == Column::bitrate ||
            col == Column::duration||
            col == Column::type||
            col == Column::file_creation_time||
            col == Column::file_modification_time) {
        return defaultFlags;
    }
    else return defaultFlags | Qt::ItemIsEditable;
}
bool BrowseTableModel::isTrackInUse(const QString& track_location) const
{
    if (PlayerInfo::instance().isFileLoaded(track_location)) return true;
    if (m_pRecordingManager->getRecordingLocation() == track_location) return true;
    return false;
}

bool BrowseTableModel::setData(const QModelIndex &_index, const QVariant &value, int /*role*/)
{
    if (!_index.isValid()) return false;
    qDebug() << "BrowseTableModel::setData(" << _index.data() << ")";
    auto row = _index.row();
    auto col = static_cast<Column>(_index.column());
    Mixxx::TrackMetadata trackMetadata;
    // set tagger information
    auto colEnum = QMetaEnum::fromType<Column>();
    for(auto i = 0; i < colEnum.keyCount(); i++)
    {
      trackMetadata.setProperty(colEnum.key(i),index(row,colEnum.value(i)).data());
    }
    trackMetadata.setProperty(colEnum.key(static_cast<int>(col)),value);
    // check if one the item were edited
    auto item = itemFromIndex(_index);
    auto track_location(getTrackLocation(_index));
    if (!writeTrackMetadataIntoFile(trackMetadata, track_location))
    {
        // Modify underlying interalPointer object
        item->setText(value.toString());
        item->setToolTip(item->text());
        return true;
    }
    else
    {
        // reset to old value in error
        item->setText(_index.data().toString());
        item->setToolTip(item->text());
        QMessageBox::critical(0, tr("Mixxx Library"),tr("Could not update file metadata.") + "\n" +track_location);
        return false;
    }
}
void BrowseTableModel::trackLoaded(QString group, TrackPointer pTrack)
{
    if (group == m_previewDeckGroup)
    {
        for (auto row = 0; row < rowCount(); ++row) {
            auto i = index(row, static_cast<int>(Column::preview));
            if (i.data().toBool())
            {
                auto item = itemFromIndex(i);
                item->setText("0");
            }
        }
        if (pTrack)
        {
            for (auto row = 0; row < rowCount(); ++row)
            {
                auto i = index(row, static_cast<int>(Column::preview));
                auto location = index(row, static_cast<int>(Column::location)).data().toString();
                if (location == pTrack->getLocation())
                {
                    auto item = itemFromIndex(i);
                    item->setText("1");
                    break;
                }
            }
        }
    }
}
bool BrowseTableModel::isColumnSortable(int column)
{
    if (Column::preview == static_cast<Column>(column)) return false; 
    return true;
}
QAbstractItemDelegate* BrowseTableModel::delegateForColumn(const int i, QObject* pParent)
{
    if (PlayerManager::numPreviewDecks() > 0 && static_cast<Column>(i) == Column::preview) return new PreviewButtonDelegate(pParent, i);
    return nullptr;
}
