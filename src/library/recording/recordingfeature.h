// recordingfeature.h
// Created 03/26/2010 by Tobias Rafreider

_Pragma("once")
#include <QStringListModel>
#include <QSortFilterProxyModel>

#include "configobject.h"
#include "library/browse/browsetablemodel.h"
#include "library/browse/foldertreemodel.h"
#include "library/libraryfeature.h"
#include "library/proxytrackmodel.h"
#include "recording/recordingmanager.h"

class Library;
class TrackCollection;

class RecordingFeature : public LibraryFeature {
    Q_OBJECT
  public:
    RecordingFeature(
        Library* parent,ConfigObject<ConfigValue>* pConfig,
        TrackCollection* pTrackCollection,RecordingManager* pRecordingManager
      );
    virtual ~RecordingFeature();
    QVariant title();
    QIcon getIcon();
    void bindWidget(WLibrary* libraryWidget,QObject* keyboard);
    TreeItemModel* getChildModel();
  public slots:
    void activate();
  signals:
    void setRootIndex(const QModelIndex&);
    void requestRestoreSearch();
    void refreshBrowseModel();
  private:
    ConfigObject<ConfigValue>* m_pConfig;
    Library* m_pLibrary;
    TrackCollection* m_pTrackCollection;
    FolderTreeModel m_childModel;
    const static QString m_sRecordingViewName;
    RecordingManager* m_pRecordingManager;
};
