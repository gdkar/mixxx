#include <QList>
#include <QtDebug>
#include <QKeyEvent>
#include <QEvent>

#include "controllers/keyboard/keyboardeventfilter.h"
#include "control/controlobject.h"
#include "util/cmdlineargs.h"

KeyboardEventFilter::KeyboardEventFilter(ConfigObject<ConfigValueKbd>* pKbdConfigObject,
                                         QObject* parent, const char* name)
        : QObject(parent),
          m_pKbdConfigObject(nullptr) {
    setObjectName(name);
    setKeyboardConfig(pKbdConfigObject);
}
KeyboardEventFilter::~KeyboardEventFilter() = default;
bool KeyboardEventFilter::eventFilter(QObject*, QEvent* e)
{
    if (e->type() == QEvent::FocusOut) {
        // If we lose focus, we need to clear out the active key list
        // because we might not get Key Release events.
        m_qActiveKeyList.clear();
    } else if (e->type() == QEvent::KeyPress) {
        auto ke = (QKeyEvent *)e;
#ifdef __APPLE__
        // On Mac OSX the nativeScanCode is empty (const 1) http://doc.qt.nokia.com/4.7/qkeyevent.html#nativeScanCode
        // We may loose the release event if a the shift key is pressed later
        // and there is character shift like "1" -> "!"
        int keyId = ke->key();
#else
        int keyId = ke->nativeScanCode();
#endif
        //qDebug() << "KeyPress event =" << ke->key() << "KeyId =" << keyId;
        // Run through list of active keys to see if the pressed key is already active
        // Just for returning true if we are consuming this key event
        for(auto && keyDownInfo: m_qActiveKeyList) {
            if (keyDownInfo.keyId == keyId)
                return true;
        }
        auto ks = getKeySeq(ke);
        if (!ks.isEmpty()) {
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
        }
    } else if (e->type()==QEvent::KeyRelease) {
        auto ke = (QKeyEvent*)e;
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
        auto matched = false;
        // Run through list of active keys to see if the released key is active
        for (auto i = m_qActiveKeyList.size(); i-- > 0; ) {
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
        }
        return matched;
    } else if (e->type() == QEvent::KeyboardLayoutChange) {
        // This event is not fired on ubunty natty, why?
        // TODO(XXX): find a way to support KeyboardLayoutChange Bug #997811
        //qDebug() << "QEvent::KeyboardLayoutChange";
    }
    return false;
}
QKeySequence KeyboardEventFilter::getKeySeq(QKeyEvent* e)
{
    auto modseq = QString{};
    QKeySequence k;
    // TODO(XXX) check if we may simply return QKeySequence(e->modifiers()+e->key())
    if (e->modifiers() & Qt::ShiftModifier)     modseq += "Shift+";
    if (e->modifiers() & Qt::ControlModifier)   modseq += "Ctrl+";
    if (e->modifiers() & Qt::AltModifier)       modseq += "Alt+";
    if (e->modifiers() & Qt::MetaModifier)      modseq += "Meta+";
    if (e->key() >= 0x01000020 && e->key() <= 0x01000023) {
        // Do not act on Modifier only
        // avoid returning "khmer vowel sign ie (U+17C0)"
        return k;
    }
    auto keyseq = QKeySequence(e->key()).toString();
    k = QKeySequence(modseq + keyseq);
    if (CmdlineArgs::Instance().getDeveloper()) {
        qDebug() << "keyboard press: " << k.toString();
    }
    return k;
}
void KeyboardEventFilter::setKeyboardConfig(ConfigObject<ConfigValueKbd>* pKbdConfigObject) {
    // Keyboard configs are a surjection from ConfigKey to key sequence. We
    // invert the mapping to create an injection from key sequence to
    // ConfigKey. This allows a key sequence to trigger multiple controls in
    // Mixxx.
    m_keySequenceToControlHash = pKbdConfigObject->transpose();
    m_pKbdConfigObject = pKbdConfigObject;
}
ConfigObject<ConfigValueKbd>* KeyboardEventFilter::getKeyboardConfig() {
    return m_pKbdConfigObject;
}
