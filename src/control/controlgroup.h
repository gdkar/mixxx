#ifndef CONTROL_CONTROLGROUP_H
#define CONTROL_CONTROLGROUP_H

#include <qatomic.h>
#include <qsharedpointer.h>
#include <qmath.h>
#include <qglobal.h>
#include <qglobalstatic.h>

class ControlGroup : public QObject {
  Q_OBJECT;
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged);
  Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged);
public:
  explicit ControlGroup(const QString &_name, QObject *pParent = 0);
  virtual ~ControlGroup();
  signals:
    void nameChanged(QString);
    void descriptionChanged(QString);
    void controlChanged(QString);
  public slots:
    virtual void setName(const QString &_name);
    virtual void setDescription(const QString &_description);
    virtual QObject *control(const QString &which);
    virtual const QString &name()const;
    virtual const QString &description()const;
    virtual void addControl(const QString &_name, QObject *obj);
  private:

    QString m_name;
    QString m_description;
};
#endif /* CONTROL_CONTROLGROUP_H */
