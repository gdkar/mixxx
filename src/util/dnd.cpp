#include "util/dnd.h"
#include "library/parser.h"
#include "library/parserm3u.h"
#include "library/parserpls.h"
#include "library/parsercsv.h"
#include <QRegExp>
#include <QScopedPointer>
#include "soundsourceproxy.h"
#include "playermanager.h"
#include "util/sandbox.h"
#include "controlobject.h"
QList<QFileInfo> DragAndDropHelper::supportedTracksFromUrls(QList<QUrl> urls,bool firstOnly,bool acceptPlaylists)
{
    QList<QFileInfo> fileLocations;
    for(auto url: urls)
    {
        // XXX: Possible WTF alert - Previously we thought we needed
        // toString() here but what you actually want in any case when
        // converting a QUrl to a file system path is
        // QUrl::toLocalFile(). This is the second time we have flip-flopped
        // on this, but I think toLocalFile() should work in any
        // case. toString() absolutely does not work when you pass the
        // result to a (this comment was never finished by the original
        // author).
        auto file = url.toLocalFile();
        // If the file is on a network share, try just converting the URL to
        // a string...
        if (file.isEmpty()) file = url.toString();
        if (file.isEmpty()) continue;
        if (acceptPlaylists && (file.endsWith(".m3u") || file.endsWith(".m3u8")))
        {
            QScopedPointer<ParserM3u> playlist_parser(new ParserM3u());
            QList<QString> track_list = playlist_parser->parse(file);
            for(auto& playlistFile: track_list) {
                addFileToList(playlistFile, &fileLocations);
            }
        } else if (acceptPlaylists && url.toString().endsWith(".pls")) {
            QScopedPointer<ParserPls> playlist_parser(new ParserPls());
            QList<QString> track_list = playlist_parser->parse(file);
            for(auto & playlistFile: track_list) {
                addFileToList(playlistFile, &fileLocations);
            }
        } else {addFileToList(file, &fileLocations);}
        if (firstOnly && !fileLocations.isEmpty()) {break;}
    }
    return fileLocations;
}
bool DragAndDropHelper::allowLoadToPlayer(QString group,ConfigObject<ConfigValue>* pConfig)
{
    return allowLoadToPlayer(
            group, ControlObject::get(ConfigKey(group, "play")) > 0.0,
            pConfig);
}
bool DragAndDropHelper::allowLoadToPlayer(QString group,bool isPlaying,ConfigObject<ConfigValue>* pConfig)
{
    // Always allow loads to preview decks.
    if (PlayerManager::isPreviewDeckGroup(group)) return true;
    return !isPlaying || pConfig->getValueString(ConfigKey("Controls","AllowTrackLoadToPlayingDeck")).toInt();
}
bool DragAndDropHelper::dragEnterAccept(const QMimeData& mimeData,QString sourceIdentifier,bool firstOnly,bool acceptPlaylists)
{
auto files = dropEventFiles(mimeData, sourceIdentifier,firstOnly, acceptPlaylists);
    return !files.isEmpty();
}
QList<QFileInfo> DragAndDropHelper::dropEventFiles(const QMimeData& mimeData,
                                           QString sourceIdentifier,
                                           bool firstOnly,
                                           bool acceptPlaylists)
{
    if (!mimeData.hasUrls() || (mimeData.hasText() && mimeData.text() == sourceIdentifier))return QList<QFileInfo>();
    return  supportedTracksFromUrls(mimeData.urls(), firstOnly, acceptPlaylists);
}
QDrag* DragAndDropHelper::dragTrack(TrackPointer pTrack, QWidget* pDragSource,QString sourceIdentifier)
{
    QList<QUrl> locationUrls;
    locationUrls.append(urlFromLocation(pTrack->getLocation()));
    return dragUrls(locationUrls, pDragSource, sourceIdentifier);
}
QDrag* DragAndDropHelper::dragTrackLocations(QList<QString> locations,
                                  QWidget* pDragSource,
                                  QString sourceIdentifier)
{
    QList<QUrl> locationUrls;
    for(auto location: locations) {locationUrls.append(urlFromLocation(location));}
    return dragUrls(locationUrls, pDragSource, sourceIdentifier);
}
QUrl DragAndDropHelper::urlFromLocation(QString trackLocation)
{
    return QUrl::fromLocalFile(trackLocation);
}
QDrag* DragAndDropHelper::dragUrls(QList<QUrl> locationUrls, QWidget* pDragSource, QString sourceIdentifier)
{
    if (locationUrls.isEmpty()) return nullptr;
    auto mimeData = new QMimeData();
    mimeData->setUrls(locationUrls);
    mimeData->setText(sourceIdentifier);
    auto drag = new QDrag(pDragSource);
    drag->setMimeData(mimeData);
    drag->setPixmap(QPixmap(":/images/library/ic_library_drag_and_drop.png"));
    drag->exec(Qt::CopyAction);
    return drag;
}
bool DragAndDropHelper::addFileToList(QString file, QList<QFileInfo>* files)
{
    auto fileInfo = QFileInfo(file);
    // Since the user just dropped these files into Mixxx we have permission
    // to touch the file. Create a security token to keep this permission
    // across reboots.
    Sandbox::createSecurityToken(fileInfo);
    if (!fileInfo.exists()) return false;
    // Filter out invalid URLs (eg. files that aren't supported audio
    // filetypes, etc.)
    if (!SoundSourceProxy::isFileSupported(fileInfo)) return false;
    files->append(fileInfo);
    return true;
}
