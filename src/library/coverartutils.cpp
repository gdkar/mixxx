#include <QDir>
#include <QDirIterator>

#include "library/coverartutils.h"

#include "soundsourceproxy.h"
#include "util/regex.h"


//static
QString CoverArtUtils::defaultCoverLocation() {
    return QString(":/images/library/cover_default.png");
}

//static
QStringList CoverArtUtils::supportedCoverArtExtensions() {
    QStringList extensions;
    extensions << "jpg" << "jpeg" << "png" << "gif" << "bmp";
    return extensions;
}

//static
QString CoverArtUtils::supportedCoverArtExtensionsRegex() {
    QStringList extensions = supportedCoverArtExtensions();
    return RegexUtils::fileExtensionsRegex(extensions);
}

//static
QImage CoverArtUtils::extractEmbeddedCover(QString trackLocation,SecurityTokenPointer pToken) {
    auto proxy = SoundSourceProxy(trackLocation, pToken);
    QImage coverArt;
    if (proxy.parseTrackMetadataAndCoverArt(nullptr, &coverArt)) return coverArt;
    else  return QImage();
}

//static
QImage CoverArtUtils::loadCover(const CoverInfo& info) {
    if (info.type == CoverInfo::METADATA) {
        if (info.trackLocation.isEmpty()) {
            qDebug() << "CoverArtUtils::loadCover METADATA cover with empty trackLocation.";
            return QImage();
        }
        SecurityTokenPointer pToken = Sandbox::openSecurityToken(
            QFileInfo(info.trackLocation), true);
        return CoverArtUtils::extractEmbeddedCover(info.trackLocation,
                                                   pToken);
    } else if (info.type == CoverInfo::FILE) {
        if (info.trackLocation.isEmpty()) {
            qDebug() << "CoverArtUtils::loadCover FILE cover with empty trackLocation."
                     << "Relative paths will not work.";
            auto pToken = Sandbox::openSecurityToken( QFileInfo(info.coverLocation), true);
            return QImage(info.coverLocation);
        }
        auto track = QFileInfo(info.trackLocation);
        auto cover = QFileInfo(track.dir(), info.coverLocation);
        if (!cover.exists()) {
            qDebug() << "CoverArtUtils::loadCover FILE cover does not exist:"
                     << info.coverLocation << info.trackLocation;
            return QImage();
        }
        auto coverPath = cover.filePath();
        auto pToken = Sandbox::openSecurityToken( cover, true);
        return QImage(coverPath);
    } else {
        qDebug() << "CoverArtUtils::loadCover bad type";
        return QImage();
    }
}

//static
CoverArt CoverArtUtils::guessCoverArt(TrackPointer pTrack) {
    CoverArt art;
    art.info.source = CoverInfo::GUESSED;
    if (pTrack.isNull()) return art;
    auto trackInfo = pTrack->getFileInfo();
    auto trackLocation = trackInfo.absoluteFilePath();

    art.image = extractEmbeddedCover(trackLocation, pTrack->getSecurityToken());
    if (!art.image.isNull()) {
        art.info.hash = calculateHash(art.image);
        art.info.coverLocation = QString();
        art.info.type = CoverInfo::METADATA;
        qDebug() << "CoverArtUtils::guessCoverArt found metadata art" << art;
        return art;
    }
    auto possibleCovers = findPossibleCoversInFolder( trackInfo.absolutePath());
    art = selectCoverArtForTrack(pTrack.data(), possibleCovers);
    if (art.info.type == CoverInfo::FILE)
        qDebug() << "CoverArtUtils::guessCoverArt found file art" << art;
    else
        qDebug() << "CoverArtUtils::guessCoverArt didn't find art" << art;
    return art;
}

//static
QLinkedList<QFileInfo> CoverArtUtils::findPossibleCoversInFolder(QString folder) {
    // Search for image files in the track directory.
    QRegExp coverArtFilenames(supportedCoverArtExtensionsRegex(), Qt::CaseInsensitive);
    QDirIterator it(folder, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    QFile currentFile;
    QFileInfo currentFileInfo;
    QLinkedList<QFileInfo> possibleCovers;
    while (it.hasNext()) {
        it.next();
        currentFileInfo = it.fileInfo();
        if (currentFileInfo.isFile() &&
            coverArtFilenames.indexIn(currentFileInfo.fileName()) != -1) {
            possibleCovers.append(currentFileInfo);
        }
    }
    return possibleCovers;
}

//static
CoverArt CoverArtUtils::selectCoverArtForTrack(TrackInfoObject* pTrack,QLinkedList<QFileInfo> covers) {
    if (!pTrack || covers.isEmpty()) {
        CoverArt art;
        art.info.source = CoverInfo::GUESSED;
        return art;
    }
    QString trackBaseName = pTrack->getFileInfo().baseName();
    QString albumName = pTrack->getAlbum();
    return selectCoverArtForTrack(trackBaseName, albumName, covers);
}
//static
CoverArt CoverArtUtils::selectCoverArtForTrack(
        QString trackBaseName,
        QString albumName,
        QLinkedList<QFileInfo> covers) {
    CoverArt art;
    art.info.source = CoverInfo::GUESSED;
    if (covers.isEmpty()) return art;
    PreferredCoverType bestType = NONE;
    auto bestInfo = static_cast<const QFileInfo*>(nullptr);
    // If there is a single image then we use it unconditionally. Otherwise
    // we use the priority order described in PreferredCoverType. Notably,
    // if there are multiple image files in the folder we require they match
    // one of the name patterns -- otherwise we run the risk of picking an
    // arbitrary image that happens to be in the same folder as some of the
    // user's music files.
    if (covers.size() == 1) {
        bestInfo = &covers.first();
    } else {
        // TODO(XXX) Sort instead so that we can fall-back if one fails to
        // open?
        for(auto &file: covers) {
            QString coverBaseName = file.baseName();
            if (bestType > TRACK_BASENAME &&
                coverBaseName.compare(trackBaseName,Qt::CaseInsensitive) == 0) {
                bestType = TRACK_BASENAME;
                bestInfo = &file;
                // This is the best type so we know we're done.
                break;
            } else if (bestType > ALBUM_NAME &&
                       coverBaseName.compare(albumName,Qt::CaseInsensitive) == 0) {
                bestType = ALBUM_NAME;
                bestInfo = &file;
            } else if (bestType > COVER && !coverBaseName.compare(QLatin1String("cover"), Qt::CaseInsensitive)) {
                bestType = COVER;
                bestInfo = &file;
            } else if (bestType > FRONT && !coverBaseName.compare(QLatin1String("front"), Qt::CaseInsensitive)) {
                bestType = FRONT;
                bestInfo = &file;
            } else if (bestType > ALBUM && !coverBaseName.compare(QLatin1String("album"), Qt::CaseInsensitive)) {
                bestType = ALBUM;
                bestInfo = &file;
            } else if (bestType > FOLDER && !coverBaseName.compare(QLatin1String("folder"), Qt::CaseInsensitive)) {
                bestType = FOLDER;
                bestInfo = &file;
            }
        }
    }
    if (bestInfo) {
        art.image = QImage(bestInfo->filePath());
        if (!art.image.isNull()) {
            art.info.source = CoverInfo::GUESSED;
            art.info.type = CoverInfo::FILE;
            art.info.hash = CoverArtUtils::calculateHash(art.image);
            art.info.coverLocation = bestInfo->fileName();
            return art;
        }
    }
    return art;
}
CoverArtUtils::CoverArtUtils() = default;
quint16 CoverArtUtils::calculateHash(const QImage& image) {
    return qChecksum(reinterpret_cast<const char*>(image.constBits()), image.byteCount());
}
