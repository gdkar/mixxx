// setlogfeature.h

#ifndef SETLOGFEATURE_H
#define SETLOGFEATURE_H

#include <QLinkedList>
#include <QSqlTableModel>
#include <QAction>

#include "library/baseplaylistfeature.h"
#include "configobject.h"

class TrackCollection;
class TreeItem;

class SetlogFeature : public BasePlaylistFeature {
    Q_OBJECT
public:
    SetlogFeature(QObject* parent, ConfigObject<ConfigValue>* pConfig,
                  TrackCollection* pTrackCollection);
    virtual ~SetlogFeature();

    QVariant title();
    QIcon getIcon();

    virtual void bindWidget(WLibrary* libraryWidget,
                            MixxxKeyboard* keyboard);

  public slots:
    void onRightClick(const QPoint& globalPos);
    void onRightClickChild(const QPoint& globalPos, QModelIndex index);
    void onJoinWithPrevious();
    void onGetNewPlaylist();

  protected:
    void buildPlaylistList();
    void decorateChild(TreeItem *pChild, int playlist_id);

  private slots:
    void onPlayingTrackChanged(TrackPointer currentPlayingTrack);
    void onPlaylistTableChanged(int playlistId);

  private:
    QString getRootViewHtml() const;

    QLinkedList<int> m_recentTracks;
    QAction* m_pJoinWithPreviousAction;
    QAction* m_pGetNewPlaylist;
    int m_playlistId;
};

#endif // SETLOGFEATURE_H
