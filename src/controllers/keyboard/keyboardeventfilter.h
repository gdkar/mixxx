#ifndef CONTROLLERS_KEYBOARD_KEYBOARDEVENTFILTER_H
#define CONTROLLERS_KEYBOARD_KEYBOARDEVENTFILTER_H

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QMultiHash>
#include "util/singleton.h"
#include "preferences/configobject.h"
#include "controllers/keyproxy.h"
class ControlObject;
struct KeyDownInformation {
    KeyDownInformation() = default;
    KeyDownInformation(int key, int modifiers, KeyProxy *proxy)
            : key(key),
                modifiers(modifiers),
                proxy(proxy)
    { }
    KeyDownInformation(QKeyEvent *evt, KeyProxy *proxy)
    : key(evt->key())
    , modifiers(evt->modifiers())
    , text(evt->text())
    , count(evt->count())
    , proxy(proxy) {}
    int key;
    Qt::KeyboardModifiers modifiers{};
    QString               text {};
    uint16_t              count{0};
    KeyProxy             *proxy{};
};
Q_DECLARE_METATYPE(KeyDownInformation);

// This class provides handling of keyboard events.
class KeyboardEventFilter : public QObject, public Singleton<KeyboardEventFilter> {
    Q_OBJECT
    Q_PROPERTY(QObject * keyboardHandler READ keyboardHandler WRITE setKeyboardHandler
        NOTIFY keyboardHandlerChanged)

  public:
    KeyboardEventFilter(QObject *parent = nullptr);
    virtual ~KeyboardEventFilter();
    bool eventFilter(QObject* obj, QEvent* e);
    QObject *keyboardHandler() const;
    void setKeyboardHandler(QObject *);
    // Set the keyboard config object. KeyboardEventFilter does NOT take
    // ownership of pKbdConfigObject.
    void setKeyboardConfig(ConfigObject<ConfigValueKbd> *pKbdConfigObject);
    ConfigObject<ConfigValueKbd>* getKeyboardConfig();
    Q_INVOKABLE KeyProxy *getBindingFor(QString _prefix);
    Q_INVOKABLE KeyProxy *getBindingFor(int _prefix);
  signals:
    void keyboardHandlerChanged(QObject*);
  private:
    QObject *m_keyboardHandler{};
    using KeyInfo = std::pair<quint32,quint32>;
    QHash<QKeySequence, KeyProxy*> m_dispatchBySequence{};
    QHash<quint32, KeyProxy*>          m_dispatchByScanCode{};
    QHash<quint32, KeyDownInformation>      m_activeKeys{};
    // Returns a valid QString with modifier keys from a QKeyEvent
    QKeySequence getKeySeq(QKeyEvent *e);
    // List containing keys which is currently pressed
    QList<KeyDownInformation> m_qActiveKeyList;
    // Pointer to keyboard config object
    ConfigObject<ConfigValueKbd> *m_pKbdConfigObject;
    // Multi-hash of key sequence to
    QMultiHash<ConfigValueKbd, ConfigKey> m_keySequenceToControlHash;
};
#endif  // CONTROLLERS_KEYBOARD_KEYBOARDEVENTFILTER_H
