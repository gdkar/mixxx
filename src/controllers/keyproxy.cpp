#include "controllers/keyproxy.h"

KeyProxy::KeyProxy(QObject *p)
: BindProxy(p) {}

KeyProxy::~KeyProxy() = default;
KeyProxy::KeyProxy(QString pre,  QObject *p)
: BindProxy(pre,p)
, m_keySequence(QKeySequence::fromString(pre))
{ }
KeyProxy::KeyProxy(QKeySequence pre,  QObject *p)
: BindProxy(pre.toString(),p)
, m_keySequence(pre)
{ }

KeyProxy::KeyProxy(Qt::Key _key, Qt::KeyboardModifiers _modifiers, QObject *p)
: BindProxy(p)
, m_keySequence(QKeySequence(_key + _modifiers))
{
    setPrefix(m_keySequence.toString());
}
KeyProxy::KeyProxy(int _scanCode,QObject *p)
: BindProxy(p)
, m_nativeScanCode(_scanCode)
{ }

QKeySequence KeyProxy::keySequence() const
{
    return m_keySequence;
}
int KeyProxy::nativeScanCode() const
{
    return m_nativeScanCode;
}
void KeyProxy::AppendTarget(QQmlListProperty<QObject> *list, QObject *item)
{
    if(auto context = qobject_cast<KeyProxy*>(list->object)) {
        context->m_targets.push_back(item);
    }
}
void KeyProxy::ClearTargets(QQmlListProperty<QObject> *list)
{
    if(auto context = qobject_cast<KeyProxy*>(list->object)){
        context->m_targets.clear();
    }
}
int KeyProxy::TargetCount(QQmlListProperty<QObject> *list)
{
    if(auto context = qobject_cast<KeyProxy*>(list->object)) {
        return context->m_targets.size();
    }
    return 0;
}
QObject *KeyProxy::TargetAt(QQmlListProperty<QObject>*list, int idx)
{
    if(auto context = qobject_cast<KeyProxy*>(list->object)){
        return context->m_targets.at(idx);
    }
    return nullptr;
}
QQmlListProperty<QObject > KeyProxy::targets()
{
    return QQmlListProperty<QObject>(this, nullptr
        , &KeyProxy::AppendTarget
        , &KeyProxy::TargetCount
        , &KeyProxy::TargetAt
        , &KeyProxy::ClearTargets);
}

bool KeyProxy::event(QEvent *ev)
{
    if(!m_targets.size())
        return false;
    if(ev->type() == QEvent::KeyPress || ev->type() == QEvent::KeyRelease) {
        for(auto it = m_targets.begin(); it != m_targets.end();){
            if(auto target = *it) {
                if( QCoreApplication::sendEvent(target, ev))
                    return true;
                ++it;
            }else{
                it = m_targets.erase(it);
            }
        }
    }
    return false;
}
