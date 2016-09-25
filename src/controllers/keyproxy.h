#ifndef CONTROLLERS_KEYBOARDPROXY_H
#define CONTROLLERS_KEYBOARDPROXY_H
#include <QtGlobal>
#include <QtQml>
#include <QtQuick>
#include <QKeyEvent>
#include <QQmlListProperty>

#include "controllers/bindproxy.h"

class KeyProxy : public BindProxy {
    Q_OBJECT
    Q_PROPERTY(int nativeScanCode READ nativeScanCode CONSTANT);
    Q_PROPERTY(QKeySequence keySequence READ keySequence CONSTANT);
    Q_PROPERTY(QQmlListProperty<QObject> targets READ targets);
public:
    Q_INVOKABLE KeyProxy(QObject *p = nullptr);
    Q_INVOKABLE KeyProxy(QString prefix, QObject *p = nullptr);
    Q_INVOKABLE KeyProxy(QKeySequence prefix, QObject *p = nullptr);
    Q_INVOKABLE KeyProxy(int _scanCode, QObject *p = nullptr);
    Q_INVOKABLE KeyProxy(Qt::Key _key, Qt::KeyboardModifiers _modifiers, QObject *p = nullptr);

    QQmlListProperty<QObject> targets();
   ~KeyProxy();
    int nativeScanCode() const;
    QKeySequence keySequence() const;
    bool event(QEvent *ev) override;
signals:
    void pressed(QKeyEvent event);
    void released(QKeyEvent event);
protected:
    std::vector<QPointer<QObject> > m_targets{};
    QKeySequence m_keySequence{};
    int m_nativeScanCode{-1};
    static void AppendTarget(QQmlListProperty<QObject> *, QObject*);
    static void ClearTargets(QQmlListProperty<QObject> *);
    static int TargetCount(QQmlListProperty<QObject> *);
    static QObject * TargetAt(QQmlListProperty<QObject> *,int );
};
QML_DECLARE_TYPE(KeyProxy)

#endif
