/**
  * @file bulkcontroller.h
  * @author Neale Picket  neale@woozle.org
  * @date Thu Jun 28 2012
  * @brief USB Kbd controller backend
  */

#ifndef KBDCONTROLLER_H
#define KBDCONTROLLER_H

#include <QAtomicInt>
#include <QEvent>
#include <QKeyEvent>
#include <QHash>
#include <QByteArray>
#include "controllers/controller.h"
#include "controllers/hid/hidcontrollerpreset.h"
#include "controllers/hid/hidcontrollerpresetfilehandler.h"

class MixxxKeyboard: public QObject {
    Q_OBJECT
  public:
    MixxxKeyboard( );
    virtual ~MixxxKeyboard();
    void stop();
    bool eventFilter(QObject *obj, QEvent *e);  
    ConfigObject<ConfigValueKbd>*getKeyboardConfig(){
      return m_pKbdConfig;
    }
  signals:
    void incomingData(QByteArray data);
  protected:
  private:
    struct KeyRec{
      static KeyRec *fromKeyEvent(QKeyEvent *e){
        KeyRec *rec = new KeyRec();
#ifdef __APPLE__
        rec->keyId     = e->key();
#else
        rec->keyId     = e->nativeScanCode();
#endif
        rec->modifiers = e->modifiers();
        rec->type      = e->type();
        return rec;
      }
      QByteArray data(){
        QByteArray ret;
        int keyOut = keyId|modifiers;
        ret.append((const char*)&keyOut,sizeof(keyId));
        ret.append((const char*)&type,sizeof(type));
        return ret;
      }
      int keyId;
      int modifiers;
      int type;
    };
    QKeySequence getKeySeq(QKeyEvent *e);
    ConfigObject<ConfigValueKbd> *m_pKbdConfig;
    QHash<int, KeyRec> m_qActiveKeyHash;
    QAtomicInt m_stop;
};

class KbdController : public Controller {
    Q_OBJECT
      using Controller::receive;
  public:
    KbdController();
    virtual ~KbdController();
    virtual QString presetExtension();
    virtual ControllerPresetPointer getPreset() const {
        HidControllerPreset* pClone = new HidControllerPreset();
        *pClone = m_preset;
        return ControllerPresetPointer(pClone);
    }
    virtual bool savePreset(const QString fileName) const;
    virtual void visit(const MidiControllerPreset* preset);
    virtual void visit(const HidControllerPreset* preset);
    virtual void accept(ControllerVisitor* visitor) {
        if (visitor) {
            visitor->visit(this);
        }
    }
    virtual bool isMappable() const {
        return m_preset.isMappable();
    }
    virtual bool matchPreset(const PresetInfo& preset);
    virtual bool matchProductInfo(QHash <QString,QString >);
    static MixxxKeyboard *getKeyboard();
  protected:
    Q_INVOKABLE void send(QList<int> data, unsigned int length);
  private slots:
    int open();
    int close();
  private:
    // For devices which only support a single report, reportID must be set to
    // 0x0.
    virtual void send(QByteArray data);
    virtual bool isPolling() const {
        return false;
    }
    // Returns a pointer to the currently loaded controller preset. For internal
    // use only.
    virtual ControllerPreset* preset() {
        return &m_preset;
    }

    // Local copies of things we need from desc
    static MixxxKeyboard *s_pKeyboard;
    MixxxKeyboard *m_pReader;
    HidControllerPreset m_preset;
};

#endif
