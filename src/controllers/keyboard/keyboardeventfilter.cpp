#include <QList>
#include <QtDebug>
#include <QKeyEvent>
#include <QEvent>

#include "controllers/keyboard/keyboardeventfilter.h"
#include "control/controlobject.h"
#include "util/cmdlineargs.h"

/*KeyboardEventFilter::KeyboardEventFilter(ConfigObject<ConfigValueKbd>* pKbdConfigObject,
                                         QObject* parent, const char* name)
        : QObject(parent),
          m_pKbdConfigObject(nullptr) {
    setObjectName(name);
    setKeyboardConfig(pKbdConfigObject);
}*/
KeyboardEventFilter::KeyboardEventFilter(QObject *pParent)
:QObject(pParent) {}
KeyboardEventFilter::~KeyboardEventFilter() = default;
bool KeyboardEventFilter::eventFilter(QObject*, QEvent* e)
{
    if (e->type() == QEvent::FocusOut) {
        // If we lose focus, we need to clear out the active key list
        // because we might not get Key Release events.
        m_qActiveKeyList.clear();
        m_activeKeys.clear();
    } else if (e->type() == QEvent::KeyPress) {
        auto ke = static_cast<QKeyEvent *>(e);
//        if(auto obj = keyboardHandler()){
//#ifdef __APPLE__
        // On Mac OSX the nativeScanCode is empty (const 1) http://doc.qt.nokia.com/4.7/qkeyevent.html#nativeScanCode
        // We may loose the release event if a the shift key is pressed later
        // and there is character shift like "1" -> "!"
#ifdef __APPLE__
        int keyId = ke->key();
#else
        int keyId = ke->nativeScanCode();
#endif
        auto ks = getKeySeq(ke);
        if(!ke->isAutoRepeat()) {
            auto info = m_activeKeys.take(keyId);
            if(info.proxy) {
                auto kue = QKeyEvent(QEvent::KeyRelease,
                    info.key,
                    info.modifiers,
                    info.text,
                    false,
                    info.count);
                QCoreApplication::sendEvent(info.proxy,&kue);
            }
        }
        if(auto obj = m_dispatchBySequence.value(ks,nullptr)) {
            if(QCoreApplication::sendEvent(obj, e)) {
                m_activeKeys.insert(keyId, KeyDownInformation(ke,obj));
                return true;
            }
        }
//        }

        //qDebug() << "KeyPress event =" << ke->key() << "KeyId =" << keyId;
        // Run through list of active keys to see if the pressed key is already active
        // Just for returning true if we are consuming this key event
/*        for(auto && keyDownInfo: m_qActiveKeyList) {
            if (keyDownInfo.keyId == keyId)
                return true;
        }
        if(auto proxy = m_dispatchBySequence.value(ks,nullptr)) {
            m_activeKeys.insert(keyId, proxy);
            proxy->pressed(ke);
            proxy->keyReceived(ke);
            proxy->setValue(1.);
        }
        if(auto proxy = m_dispatchByScanCode.value(keyId,nullptr)) {
            m_activeKeys.insert(keyId, proxy);
            proxy->pressed(ke);
            proxy->keyReceived(ke);
            proxy->setValue(1.);
        }*/
/*        if (!ks.isEmpty()) {
            ConfigValueKbd ksv(ks);
            // Check if a shortcut is defined
            auto result = false;
            // using const_iterator here is faster than QMultiHash::values()
            for(auto && configKey : m_keySequenceToControlHash.values(ksv)) {
                if (configKey.group != "[KeyboardShortcuts]") {
                    auto control = ControlObject::getControl(configKey);
                    if (control) {
                        //qDebug() << configKey << "MIDI_NOTE_ON" << 1;
                        // Add key to active key list
                        m_qActiveKeyList.append(KeyDownInformation(keyId, ke->modifiers(), control));
                        // Since setting the value might cause us to go down
                        // a route that would eventually clear the active
                        // key list, do that last.
                        control->setValueFromMidi(MIDI_NOTE_ON, 1);
                        result = true;
                    } else {
                        qDebug() << "Warning: Keyboard key is configured for nonexistent control:"
                                 << configKey.group << configKey.item;
                    }
                }
            }
            return result;
        }*/
        return false;
    } else if (e->type()==QEvent::KeyRelease) {
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

        for(auto obj : m_activeKeys.values(keyId)) {
            if(QCoreApplication::sendEvent(obj.proxy, ke))
                handled = true;
        }
        m_activeKeys.remove(keyId);
/*        if(auto obj = m_dispatchBySequence.value(ks,nullptr)){
            if(QCoreApplication::sendEvent(obj, e))
                handled = true;;
        }*/
        if(handled) {
            return true;
        }
        return false;
/*        if(auto obj = keyboardHandler()){
            if(QCoreApplication::sendEvent(obj, e))
                return true;
        }*/
/*        auto matched = false;
        for(auto && proxy : m_activeKeys.values(keyId)) {
            proxy->released(*ke);
            proxy->setValue(0.);
        }
        m_activeKeys.remove(keyId);*/
        // Run through list of active keys to see if the released key is active
/*        for (auto i = m_qActiveKeyList.size(); i-- > 0; ) {
            const auto& keyDownInfo = m_qActiveKeyList[i];
            auto pControl = keyDownInfo.pControl;
            if (keyDownInfo.keyId == keyId || (clearModifiers > 0 && keyDownInfo.modifiers == clearModifiers)) {
                if (!autoRepeat) {
                    //qDebug() << pControl->getKey() << "MIDI_NOTE_OFF" << 0;
                    pControl->setValueFromMidi(MIDI_NOTE_OFF, 0);
                    m_qActiveKeyList.removeAt(i);
                }
                // Due to the modifier clearing workaround we might match multiple keys for
                // release.
                matched = true;
            }
        }*/
//        return matched;
    } else if (e->type() == QEvent::KeyboardLayoutChange) {
        // This event is not fired on ubunty natty, why?
        // TODO(XXX): find a way to support KeyboardLayoutChange Bug #997811
        //qDebug() << "QEvent::KeyboardLayoutChange";
    }
    return false;
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
    auto b = new KeyProxy(seq);
    QQmlEngine::setObjectOwnership(b, QQmlEngine::JavaScriptOwnership);
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
