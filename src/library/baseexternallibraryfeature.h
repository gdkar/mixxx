_Pragma("once")
#include <QAction>
#include <QModelIndex>

#include "library/libraryfeature.h"

class BaseSqlTableModel;
class TrackCollection;

class BaseExternalLibraryFeature : public LibraryFeature {
    Q_OBJECT
  public:
    BaseExternalLibraryFeature(QObject* pParent, TrackCollection* pCollection);
    virtual ~BaseExternalLibraryFeature();
  public slots:
    virtual void onRightClick(const QPoint& globalPos);
    virtual void onRightClickChild(const QPoint& globalPos, QModelIndex index);
  protected:
    // Must be implemented by external Libraries copied to Mixxx DB
    virtual BaseSqlTableModel* getPlaylistModelForPlaylist(QString /*playlist*/) { return nullptr; }
    // Must be implemented by external Libraries not copied to Mixxx DB
    virtual void appendTrackIdsFromRightClickIndex(QList<TrackId>* trackIds, QString* pPlaylist);
    QModelIndex m_lastRightClickedIndex;
  private slots:
    void slotAddToAutoDJ();
    void slotAddToAutoDJTop();
    void slotImportAsMixxxPlaylist();
  private:
    void addToAutoDJ(bool bTop);
    TrackCollection* m_pTrackCollection = nullptr;
    QAction* m_pAddToAutoDJAction = nullptr;
    QAction* m_pAddToAutoDJTopAction = nullptr;
    QAction* m_pImportAsMixxxPlaylistAction = nullptr;
};
