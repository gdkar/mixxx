#ifndef SONGDOWNLOADER_H
#define SONGDOWNLOADER_H

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QQueue>
#include <QUrl>

class SongDownloader : public QObject {
    Q_OBJECT
  public:
    SongDownloader(QObject* parent);
    virtual ~SongDownloader();

    bool downloadSongFromURL(QUrl& url);

  public slots:
    void onReadyRead();
    void onError(QNetworkReply::NetworkError error);
    void onProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    //void finishedSlot(QNetworkReply* reply);

  signals:
    void downloadProgress(qint64, qint64);
    void downloadFinished();
    void downloadError();

  private:
    bool downloadFromQueue();

    QNetworkAccessManager* m_pNetwork;
    QQueue<QUrl> m_downloadQueue;
    QFile* m_pDownloadedFile;
    QNetworkReply* m_pReply;
    QNetworkRequest* m_pRequest;
};

#endif // SONGDOWNLOADER_H
