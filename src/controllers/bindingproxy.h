#ifndef CONTROLLERS_BINDINGPROXY_H
#define CONTROLLERS_BINDINGPROXY_H
#include <QtGlobal>
#include <QtQml>
#include <QtQuick>
#include <QObject>
#include <QString>

class BindingProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QVariant prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
public:
    Q_INVOKABLE BindingProxy(QObject *p = nullptr);
    Q_INVOKABLE BindingProxy(QVarinat prefix, QObject *p);
   ~BindingProxy();
    QVariant value() const;
    void setValue(QVariant);
    QVariant prefix() const;
    void setPrefix(QVariant );
signals:
    void valueChanged(QVariant);
    void messageReceived(QVariant);
    void prefixChanged(QVariant);
protected:
    QVariant m_prefix{};
    QVariant m_value{};
};
QML_DECLARE_TYPE(BindingProxy)

#endif
