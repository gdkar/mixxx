#ifndef BASEPLAYER_H
#define BASEPLAYER_H

#include <QObject>
#include <QString>

class BasePlayer : public QObject {
    Q_OBJECT
  public:
    BasePlayer(const QString &group, QObject* pParent = nullptr);
    virtual ~BasePlayer();

    inline const QString& getGroup() {return m_group;}

  private:
    const QString m_group;
};

#endif /* BASEPLAYER_H */
