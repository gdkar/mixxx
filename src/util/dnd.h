#ifndef DND_H
#define DND_H

#include <QUrl>
#include <QDrag>
#include <QMimeData>
#include <QList>
#include <QString>
#include <QFileInfo>
#include <QRegExp>
#include <QScopedPointer>

#include "preferences/usersettings.h"
#include "control/controlobject.h"
#include "sources/soundsourceproxy.h"
#include "library/parser.h"
#include "library/parserm3u.h"
#include "library/parserpls.h"
#include "library/parsercsv.h"
#include "util/sandbox.h"
#include "mixer/playermanager.h"

namespace DragAndDropHelper {
    QList<QFileInfo> supportedTracksFromUrls(const QList<QUrl>& urls,
                                                    bool firstOnly,
                                                    bool acceptPlaylists);
    // Allow loading to a player if the player isn't playing or the settings
    // allow interrupting a playing player.
    bool allowLoadToPlayer(const QString& group,
                                  UserSettingsPointer pConfig);
    // Allow loading to a player if the player isn't playing or the settings
    // allow interrupting a playing player.
    bool allowLoadToPlayer(const QString& group,
                                  bool isPlaying,
                                  UserSettingsPointer pConfig);
    bool dragEnterAccept(const QMimeData& mimeData,
                                const QString& sourceIdentifier,
                                bool firstOnly,
                                bool acceptPlaylists);
    QList<QFileInfo> dropEventFiles(const QMimeData& mimeData,
                                           const QString& sourceIdentifier,
                                           bool firstOnly,
                                           bool acceptPlaylists);
    QDrag* dragTrack(TrackPointer pTrack, QWidget* pDragSource,
                            QString sourceIdentifier);
    QDrag* dragTrackLocations(const QList<QString>& locations,
                                     QWidget* pDragSource,
                                     QString sourceIdentifier);
    QUrl urlFromLocation(const QString& trackLocation);
};

#endif /* DND_H */
