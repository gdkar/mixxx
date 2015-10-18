_Pragma("once")
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QQueue>
#include <QUrl>
#include <string>
class SongDownloader : public QObject {
    Q_OBJECT
  public:
    SongDownloader(QObject* parent);
    virtual ~SongDownloader();
    bool downloadSongFromURL(QUrl url);
  public slots:
    void slotReadyRead();
    void slotError(QNetworkReply::NetworkError error);
    void slotProgress(qint64 bytesReceived, qint64 bytesTotal);
    void slotDownloadFinished();
    //void finishedSlot(QNetworkReply* reply);
  signals:
    void downloadProgress(qint64, qint64);
    void downloadFinished();
    void downloadError();
  private:
    bool downloadFromQueue();
    QNetworkAccessManager* m_pNetwork = nullptr;
    QQueue<QUrl> m_downloadQueue;
    QFile* m_pDownloadedFile = nullptr;
    QNetworkReply* m_pReply = nullptr;
    QNetworkRequest* m_pRequest = nullptr;
};
