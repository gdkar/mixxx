_Pragma("once")
#include <QStandardItemModel>
#include <QMimeData>

#include "library/trackmodel.h"
#include "library/trackcollection.h"
#include "recording/recordingmanager.h"
#include "util/file.h"
#include "library/browse/browsethread.h"

// The BrowseTable models displays tracks
// of given directory on the HDD.
// Usage: Recording and Browse feature.
class BrowseTableModel : public QStandardItemModel, public virtual TrackModel {
    Q_OBJECT

  public:
    enum class Column 
    {
      preview     = 0,
      filename    = 1,
      artist      = 2,
      title       = 3,
      album       = 4,
      track_number = 5,
      year        = 6,
      genre       = 7,
      composer    = 8,
      comment     = 9,
      duration    = 10,
      bpm         = 11,
      key         = 12,
      type        = 13,
      bitrate     = 14,
      location    = 15,
      album_artist = 16,
      grouping    = 17,
      file_modification_time = 18,
      file_creation_time     = 19
    };
    Q_ENUM(Column);
    BrowseTableModel(QObject* parent, TrackCollection* pTrackCollection, RecordingManager* pRec);
    virtual ~BrowseTableModel();
    void setPath(const MDir& path);
    //reimplemented from TrackModel class
    virtual TrackPointer getTrack(const QModelIndex& index) const;
    virtual TrackModel::CapabilitiesFlags getCapabilities() const;
    QString getTrackLocation(const QModelIndex& index) const;
    TrackId getTrackId(const QModelIndex& index) const;
    const QLinkedList<int> getTrackRows(TrackId trackId) const;
    void search(const QString& searchText,const QString& extraFilter=QString());
    void removeTrack(const QModelIndex& index);
    void removeTracks(const QModelIndexList& indices);
    bool addTrack(const QModelIndex& index, QString location);
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    const QString currentSearch() const;
    bool isColumnInternal(int);
    void moveTrack(const QModelIndex&, const QModelIndex&);
    bool isLocked() { return false;}
    bool isColumnHiddenByDefault(int column);
    const QList<int>& searchColumns() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role=Qt::EditRole);
    QAbstractItemDelegate* delegateForColumn(const int i, QObject* pParent);
    virtual bool isColumnSortable(int column);
  public slots:
    void slotClear(BrowseTableModel*);
    void slotInsert(const QList< QList<QStandardItem*> >&, BrowseTableModel*);
    void trackLoaded(QString group, TrackPointer pTrack);
  private:
    void removeTracks(QStringList trackLocations);
    void addSearchColumn(int index);
    bool isTrackInUse(const QString& file) const;
    QList<int> m_searchColumns;
    MDir m_current_directory;
    TrackCollection* m_pTrackCollection;
    RecordingManager* m_pRecordingManager;
    BrowseThreadPointer m_pBrowseThread;
    QString m_previewDeckGroup;
};
