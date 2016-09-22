#ifndef CONTROLLERS_BINDINGPROXY_H
#define CONTROLLERS_BINDINGPROXY_H
#include <QtGlobal>
#include <QtQml>
#include <QtQuick>
#include <QObject>
#include <QString>
class BindingProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(double value READ value NOTIFY valueChanged)
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
public:
    Q_INVOKABLE BindingProxy(QObject *p = nullptr);
    Q_INVOKABLE BindingProxy(QString prefix, QObject *p);
   ~BindingProxy();
    double value() const;
    void setValue(double);
    QString prefix() const;
    void setPrefix(QString);
signals:
    void valueChanged(double);
    void messageReceived(double, double = 0);
    void prefixChanged(QVariant);

protected:
    QString  m_prefix{};
    double   m_value{};
};
QML_DECLARE_TYPE(BindingProxy)

#endif
