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
    static QList<QFileInfo> supportedTracksFromUrls(const QList<QUrl>& urls,bool firstOnly,bool acceptPlaylists);
    // Allow loading to a player if the player isn't playing or the settings
    // allow interrupting a playing player.
    static bool allowLoadToPlayer(const QString& group,ConfigObject<ConfigValue>* pConfig);
    // Allow loading to a player if the player isn't playing or the settings
    // allow interrupting a playing player.
    static bool allowLoadToPlayer(const QString& group,bool isPlaying,ConfigObject<ConfigValue>* pConfig);
    static bool dragEnterAccept(const QMimeData& mimeData,const QString& sourceIdentifier,bool firstOnly,bool acceptPlaylists);
    static QList<QFileInfo> dropEventFiles(const QMimeData& mimeData,const QString& sourceIdentifier,bool firstOnly,bool acceptPlaylists);
    static QDrag* dragTrack(TrackPointer pTrack, QWidget* pDragSource,QString sourceIdentifier);
    static QDrag* dragTrackLocations(const QList<QString>& locations,QWidget* pDragSource,QString sourceIdentifier);
    static QUrl urlFromLocation(const QString& trackLocation);
  private:
    static QDrag* dragUrls(const QList<QUrl>& locationUrls, QWidget* pDragSource, QString sourceIdentifier);
    static bool addFileToList(const QString& file, QList<QFileInfo>* files);
    DragAndDropHelper() = delete;
};
