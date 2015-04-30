/***************************************************************************
                          mixxxkeyboard.cpp  -  description
                             -------------------
    begin                : Wed Dec 2 2003
    copyright            : (C) 2003 by Tue Haste Andersen
    email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QList>
#include <QtDebug>
#include <QKeyEvent>
#include <QEvent>

#include "mixxxkeyboard.h"
#include "control/controlobjectslave.h"
#include "util/cmdlineargs.h"


MixxxKeyboard::MixxxKeyboard(ConfigObject<ConfigValueKbd>* pKbdConfigObject,
                             QObject* parent, const char* name)
        : QObject(parent),
          m_pKbdConfigObject(NULL) {
    setObjectName(name);
    setKeyboardConfig(pKbdConfigObject);
}

MixxxKeyboard::~MixxxKeyboard() {
}

bool MixxxKeyboard::eventFilter(QObject*, QEvent* e) {
    if (e->type() == QEvent::FocusOut) {
        // If we lose focus, we need to clear out the active key list
        // because we might not get Key Release events.
        m_qActiveKeyList.clear();
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = (QKeyEvent *)e;

#ifdef __APPLE__
        // On Mac OSX the nativeScanCode is empty (const 1) http://doc.qt.nokia.com/4.7/qkeyevent.html#nativeScanCode
        // We may loose the release event if a the shift key is pressed later
        // and there is character shift like "1" -> "!"
        int keyId = ke->nativeVirtualKey();
#else
        int keyId = ke->nativeScanCode();
#endif
        //qDebug() << "KeyPress event =" << ke->key() << "KeyId =" << keyId;

        // Run through list of active keys to see if the pressed key is already active
        // Just for returning true if we are consuming this key event

        foreach (const KeyDownInformation& keyDownInfo, m_qActiveKeyList) {
            if (keyDownInfo.keyId == keyId) {return true;}
        }
        QKeySequence ks = getKeySeq(ke);
        if (!ks.isEmpty()) {
            // Check if a shortcut is defined
            bool result = false;
            for (QHash<QKeySequence, QSharedPointer<ControlObjectSlave> >::iterator it =
                         m_keysequence_to_cos.find(ks); it != m_keysequence_to_cos.end() && it.key() == ks; ++it) {
                  QSharedPointer<ControlObjectSlave> cos (it.value());
                  if(cos){
                    const ConfigKey & configkey = cos->getKey();
                    if (configkey.group != "[KeyboardShortcuts]") {
                        // Add key to active key list
                        m_qActiveKeyList.append(KeyDownInformation(keyId, ke->modifiers(), cos));
                        // Since setting the value might cause us to go down
                        // a route that would eventually clear the active
                        // key list, do that last.
                        cos->setParameter( 1);
                        result = true;
                    } else {
                        qDebug() << "Warning: Keyboard key is configured for nonexistent control:"
                                 << configkey.group << configkey.item;
                    }
                }
            }
            return result;
        }
    } else if (e->type()==QEvent::KeyRelease) {
        QKeyEvent* ke = (QKeyEvent*)e;

#ifdef __APPLE__
        // On Mac OSX the nativeScanCode is empty
        int keyId = ke->nativeVirtualKey();
#else
        int keyId = ke->nativeScanCode();
#endif
        bool autoRepeat = ke->isAutoRepeat();
        //qDebug() << "KeyRelease event =" << ke->key() << "AutoRepeat =" << autoRepeat << "KeyId =" << keyId;
        int clearModifiers = 0;
#ifdef __APPLE__
        // OS X apparently doesn't deliver KeyRelease events when you are
        // holding Ctrl. So release all key-presses that were triggered with
        // Ctrl.
        if (ke->key() == Qt::Key_Control) {clearModifiers = Qt::ControlModifier;}
#endif
        bool matched = false;
        // Run through list of active keys to see if the released key is active
        for (int i = m_qActiveKeyList.size() - 1; i >= 0; i--) {
            KeyDownInformation& keyDownInfo = m_qActiveKeyList[i];
            QSharedPointer<ControlObjectSlave> cos(keyDownInfo.cos);
            if (keyDownInfo.keyId == keyId ||
                    (clearModifiers > 0 && keyDownInfo.modifiers == clearModifiers)) {
                if (!autoRepeat) {
                    cos->setParameter(0);
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

QKeySequence MixxxKeyboard::getKeySeq(QKeyEvent* e) {
    QString modseq;
    QKeySequence k;

    // TODO(XXX) check if we may simply return QKeySequence(e->modifiers()+e->key())

    if (e->modifiers() & Qt::ShiftModifier)
        modseq += "Shift+";

    if (e->modifiers() & Qt::ControlModifier)
        modseq += "Ctrl+";

    if (e->modifiers() & Qt::AltModifier)
        modseq += "Alt+";

    if (e->modifiers() & Qt::MetaModifier)
        modseq += "Meta+";

    if (e->key() >= 0x01000020 && e->key() <= 0x01000023) {
        // Do not act on Modifier only
        // avoid returning "khmer vowel sign ie (U+17C0)"
        return k;
    }

    QString keyseq = QKeySequence(e->key()).toString();
    k = QKeySequence(modseq + keyseq);

    if (CmdlineArgs::Instance().getDeveloper()) {
        qDebug() << "keyboard press: " << k.toString();
    }
    return k;
}

void MixxxKeyboard::setKeyboardConfig(ConfigObject<ConfigValueKbd>* pKbdConfigObject) {
    // Keyboard configs are a surjection from ConfigKey to key sequence. We
    // invert the mapping to create an injection from key sequence to
    // ConfigKey. This allows a key sequence to trigger multiple controls in
    // Mixxx.
    QHash<ConfigKey, ConfigValueKbd> keyboardConfig = pKbdConfigObject->toHash();
    m_keysequence_to_cos.clear();
    for (QHash<ConfigKey, ConfigValueKbd>::iterator it = keyboardConfig.begin(); it != keyboardConfig.end(); ++it) {
        if(!m_cached_cos.contains(it.key()))
          m_cached_cos.insert(it.key(),QSharedPointer<ControlObjectSlave>(new ControlObjectSlave(it.key())));
        m_keysequence_to_cos.insert(it.value().m_qKey, m_cached_cos.value(it.key()));
    }
    m_pKbdConfigObject = pKbdConfigObject;
}

ConfigObject<ConfigValueKbd>* MixxxKeyboard::getKeyboardConfig() {return m_pKbdConfigObject;}
