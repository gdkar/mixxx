_Pragma("once")
#include <QUrl>
#include <QString>
#include <QObject>

class MixxxUrls : public QObject
{
    MixxxUrls() = delete;
  public:
    static QUrl    website();
    static QUrl    support();
    static QUrl    feedback();
    static QUrl    translation();
    static QUrl    manual();
    static QUrl    shortcuts();
    static QString manual_filename();
};
