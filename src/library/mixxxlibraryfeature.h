// mixxxlibraryfeature.h
// Created 8/23/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QStringListModel>
#include <QUrl>
#include <QVariant>
#include <QIcon>
#include <QModelIndex>
#include <QList>
#include <QString>
#include <QSharedPointer>
#include <QObject>

#include "library/libraryfeature.h"
#include "library/dao/trackdao.h"
#include "treeitemmodel.h"
#include "configobject.h"

class DlgHidden;
class DlgMissing;
class Library;
class BaseTrackCache;
class LibraryTableModel;
class TrackCollection;

class MixxxLibraryFeature : public LibraryFeature {
    Q_OBJECT
    public:
    MixxxLibraryFeature(Library* pLibrary, TrackCollection* pTrackCollection, ConfigObject<ConfigValue>* pConfig);
    virtual ~MixxxLibraryFeature();
    virtual QVariant title();
    virtual QIcon getIcon();
    virtual bool dropAccept(QList<QUrl> urls, QObject* pSource);
    virtual bool dragMoveAccept(QUrl url);
    virtual TreeItemModel* getChildModel();
    virtual void bindWidget(WLibrary* pLibrary,QObject* pKeyboard);
  public slots:
    virtual void activate();
    virtual void activateChild(const QModelIndex& index);
    virtual void refreshLibraryModels();
  private:
    const QString kMissingTitle;
    const QString kHiddenTitle;
    Library* m_pLibrary = nullptr;
    QSharedPointer<BaseTrackCache> m_pBaseTrackCache;
    LibraryTableModel* m_pLibraryTableModel = nullptr;
    DlgMissing* m_pMissingView = nullptr;
    DlgHidden* m_pHiddenView = nullptr;
    TreeItemModel m_childModel;
    TrackDAO& m_trackDao;
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    TrackCollection* m_pTrackCollection = nullptr;
};
