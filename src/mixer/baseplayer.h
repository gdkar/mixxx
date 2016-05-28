_Pragma("once")
#include <QObject>
#include <QString>

class BasePlayer : public QObject {
    Q_OBJECT
  public:
    BasePlayer(QObject* pParent, QString group);
    virtual ~BasePlayer();
    inline const QString& getGroup() {
        return m_group;
    }
  private:
    const QString m_group;
};
