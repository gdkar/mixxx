#include <QList>
#include <QtDebug>
#include <QKeyEvent>
#include <QEvent>

#include "controllers/keyboard/keyboardeventfilter.h"
#include "control/controlobject.h"
#include "util/cmdlineargs.h"

KeyboardEventFilter::KeyboardEventFilter(QObject *pParent)
:QObject(pParent) {}
KeyboardEventFilter::~KeyboardEventFilter() = default;
bool KeyboardEventFilter::eventFilter(QObject*, QEvent* e)
{
    switch ( e->type()) {
        case QEvent::FocusOut:{
            // If we lose focus, we need to clear out the active key list
            // because we might not get Key Release events.
            m_activeKeys.clear();
            return false;
        }
        case QEvent::KeyPress: {
                auto ke = static_cast<QKeyEvent *>(e);
#ifdef __APPLE__
                int keyId = ke->key();
#else
                int keyId = ke->nativeScanCode();
#endif
                auto ks = getKeySeq(ke);
                if(!ke->isAutoRepeat()) {
                    auto info = m_activeKeys.take(keyId);
                    if(info.proxy)
                        QCoreApplication::sendEvent(info.proxy,&info.event);
                }
                if(auto obj = m_dispatchBySequence.value(ks,nullptr)) {
                    if(QCoreApplication::sendEvent(obj, e)) {
                        m_activeKeys.insert(keyId, KeyDownInformation(ke,obj));
                        return true;
                    }
                }
            };
            return false;
        case QEvent::KeyRelease: {
                auto ke = static_cast<QKeyEvent*>(e);
#ifdef __APPLE__
                // On Mac OSX the nativeScanCode is empty
                auto keyId = ke->key();
#else
                auto keyId = ke->nativeScanCode();
#endif
                auto autoRepeat = ke->isAutoRepeat();
                //qDebug() << "KeyRelease event =" << ke->key() << "AutoRepeat =" << autoRepeat << "KeyId =" << keyId;

                auto clearModifiers = 0;
#ifdef __APPLE__
                // OS X apparently doesn't deliver KeyRelease events when you are
                // holding Ctrl. So release all key-presses that were triggered with
                // Ctrl.
                if (ke->key() == Qt::Key_Control) {
                    clearModifiers = Qt::ControlModifier;
                }
#endif
                auto ks = getKeySeq(ke);
                auto handled = false;
                auto proxy = static_cast<KeyProxy*>(nullptr);
                if(!autoRepeat) {
                    auto done = m_activeKeys.take(keyId);
                    if((proxy = done.proxy)) {
                        if(QCoreApplication::sendEvent(proxy, ke))
                            handled = true;
                    }
                }
                if(auto obj = m_dispatchBySequence.value(ks,nullptr)) {
                    if(obj != proxy) {
                        if(QCoreApplication::sendEvent(obj, e))
                            handled = true;
                    }
                }
                return handled;
            };
        case QEvent::KeyboardLayoutChange: return false;
        default: return false;
    }
}
QKeySequence KeyboardEventFilter::getKeySeq(QKeyEvent* e)
{
    return QKeySequence(e->modifiers() + e->key());
}
void KeyboardEventFilter::setKeyboardConfig(ConfigObject<ConfigValueKbd>* pKbdConfigObject)
{
    // Keyboard configs are a surjection from ConfigKey to key sequence. We
    // invert the mapping to create an injection from key sequence to
    // ConfigKey. This allows a key sequence to trigger multiple controls in
    // Mixxx.
    m_keySequenceToControlHash = pKbdConfigObject->transpose();
    m_pKbdConfigObject = pKbdConfigObject;
}
ConfigObject<ConfigValueKbd>* KeyboardEventFilter::getKeyboardConfig()
{
    return m_pKbdConfigObject;
}

KeyProxy *KeyboardEventFilter::getBindingFor(QString _prefix)
{
    auto seq = QKeySequence::fromString(_prefix);
    if(auto b = m_dispatchBySequence.value(seq,nullptr)) {
        return b;
    }
    auto b = new KeyProxy(seq, this);
    QQmlEngine::setObjectOwnership(b, QQmlEngine::CppOwnership);
    m_dispatchBySequence.insert(seq, b);
    return b;
}
KeyProxy *KeyboardEventFilter::getBindingFor(int _prefix)
{
    if(auto b = m_dispatchByScanCode.value(_prefix,nullptr)) {
        return b;
    }
    auto b = new KeyProxy(_prefix);
    QQmlEngine::setObjectOwnership(b, QQmlEngine::JavaScriptOwnership);
    m_dispatchByScanCode.insert(_prefix, b);
    return b;
}

QObject *KeyboardEventFilter::keyboardHandler() const
{
    return m_keyboardHandler;
}
void KeyboardEventFilter::setKeyboardHandler(QObject *p)
{
    if(p != m_keyboardHandler) {
        emit(keyboardHandlerChanged(m_keyboardHandler = p));
    }
}
