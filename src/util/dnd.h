_Pragma("once")
#include <QUrl>
#include <QDrag>
#include <QMimeData>
#include <QList>
#include <QString>
#include <QFileInfo>
#include "trackinfoobject.h"
#include "configobject.h"

class DragAndDropHelper
{
  public:
    static QList<QFileInfo> supportedTracksFromUrls(QList<QUrl> urls,bool firstOnly,bool acceptPlaylists);
    // Allow loading to a player if the player isn't playing or the settings
    // allow interrupting a playing player.
    static bool allowLoadToPlayer(QString group,ConfigObject<ConfigValue>* pConfig);
    // Allow loading to a player if the player isn't playing or the settings
    // allow interrupting a playing player.
    static bool allowLoadToPlayer(QString group,bool isPlaying,ConfigObject<ConfigValue>* pConfig);
    static bool dragEnterAccept(QMimeData mimeData,QString sourceIdentifier,bool firstOnly,bool acceptPlaylists);
    static QList<QFileInfo> dropEventFiles(QMimeData mimeData,QString sourceIdentifier,bool firstOnly,bool acceptPlaylists);
    static QDrag* dragTrack(TrackPointer pTrack, QWidget* pDragSource,QString sourceIdentifier);
    static QDrag* dragTrackLocations(QList<QString> locations,QWidget* pDragSource,QString sourceIdentifier);
    static QUrl urlFromLocation(QString trackLocation);
  private:
    static QDrag* dragUrls(QList<QUrl> locationUrls, QWidget* pDragSource, QString sourceIdentifier);
    static bool addFileToList(QString file, QList<QFileInfo>* files);
    DragAndDropHelper() = delete;
};
