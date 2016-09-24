#ifndef CONTROLLERS_BINDINGPROXY_H
#define CONTROLLERS_BINDINGPROXY_H
#include <QtGlobal>
#include <QtQml>
#include <QtQuick>
#include <QObject>
#include <QString>
class BindProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(double value READ value NOTIFY valueChanged)
    Q_PROPERTY(QString prefix READ prefix NOTIFY prefixChanged)
public:
    Q_INVOKABLE BindProxy(QObject *p = nullptr);
    Q_INVOKABLE BindProxy(QString prefix, QObject *p);
   ~BindProxy();
    virtual double value() const;
    virtual void setValue(double);
    virtual QString prefix() const;
    virtual void setPrefix(QString);
signals:
    void valueChanged(double val);
    void messageReceived(double val , double timestamp = 0);
    void prefixChanged(QString);

protected:
    QString  m_prefix{};
    double   m_value{};
};
QML_DECLARE_TYPE(BindProxy)

#endif
