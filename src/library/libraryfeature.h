// libraryfeature.h
// Created 8/17/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QtDebug>
#include <QIcon>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QAbstractItemModel>
#include <QUrl>

#include "trackinfoobject.h"
#include "treeitemmodel.h"
#include "library/coverartcache.h"
class TrackModel;
class WLibrarySidebar;
class WLibrary;
// pure virtual (abstract) class to provide an interface for libraryfeatures
class LibraryFeature : public QObject {
  Q_OBJECT
  public:
    LibraryFeature(QObject* parent = nullptr);
    virtual ~LibraryFeature();
    virtual QVariant title() = 0;
    virtual QIcon getIcon() = 0;
    virtual bool dropAccept(QList<QUrl> /*urls*/, QObject* /*pSource*/);
    virtual bool dropAcceptChild(const QModelIndex& /*index*/, QList<QUrl> /*urls*/, QObject* /*pSource*/);
    virtual bool dragMoveAccept(QUrl /*url*/);
    virtual bool dragMoveAcceptChild(const QModelIndex& /*index*/, QUrl /*url*/);
    // Reimplement this to register custom views with the library widget.
    virtual void bindWidget(WLibrary* /* libraryWidget */, QObject* /* keyboard */);
    virtual TreeItemModel* getChildModel() = 0;
  public slots:
    // called when you single click on the root item
    virtual void activate() = 0;
    // called when you single click on a child item, e.g., a concrete playlist or crate
    virtual void activateChild(const QModelIndex& /*index*/);
    // called when you right click on the root item
    virtual void onRightClick(const QPoint& /*globalPos*/);
    // called when you right click on a child item, e.g., a concrete playlist or crate
    virtual void onRightClickChild(const QPoint& /*globalPos*/, QModelIndex /*index*/);
    // Only implement this, if using incremental or lazy childmodels, see BrowseFeature.
    // This method is executed whenever you **double** click child items
    virtual void onLazyChildExpandation(const QModelIndex& /*index*/);
  signals:
    void showTrackModel(QAbstractItemModel* model);
    void switchToView(const QString& view);
    void loadTrack(TrackPointer pTrack);
    void loadTrackToPlayer(TrackPointer pTrack, QString group, bool play = false);
    void restoreSearch(const QString&);
    // emit this signal before you parse a large music collection, e.g., iTunes, Traktor.
    // The second arg indicates if the feature should be "selected" when loading starts
    void featureIsLoading(LibraryFeature*, bool selectFeature);
    // emit this signal if the foreign music collection has been imported/parsed.
    void featureLoadingFinished(LibraryFeature*s);
    // emit this signal to select pFeature
    void featureSelect(LibraryFeature* pFeature, const QModelIndex& index);
    // emit this signal to enable/disable the cover art widget
    void enableCoverArtDisplay(bool);
    void trackSelected(TrackPointer pTrack);
};
