/**
  * @file kbdcontroller.cpp
  * @author Neale Pickett  neale@woozle.org
  * @date Thu Jun 28 2012
  * @brief USB Kbd controller backend
  *
  */
#include <libusb.h>

#include "controllers/kbd/kbdcontroller.h"
#include "controllers/defs_controllers.h"
#include "util/compatibility.h"
#include "util/trace.h"

MixxxKeyboard::MixxxKeyboard()
          :m_stop(0)
          {
  m_pKbdConfig = new ConfigObject<ConfigValueKbd>("");
}
MixxxKeyboard::~MixxxKeyboard() {
  delete m_pKbdConfig;
}
bool MixxxKeyboard::eventFilter(QObject *obj, QEvent *e){
  Q_UNUSED(obj);
  if(e->type() == QEvent::FocusOut){
    qDebug() << "got focus out; flushing all the keys!";
    foreach(KeyRec rec,m_qActiveKeyHash){
      rec.type = QEvent::KeyRelease;
      emit(incomingData(rec.data()));      
    }
    m_qActiveKeyHash.clear();
  }
  else if(e->type() == QEvent::KeyPress || e->type()==QEvent::KeyRelease){
    QKeyEvent *ke = (QKeyEvent*)e;
    KeyRec *rec = KeyRec::fromKeyEvent(ke);
    if(rec->type==QEvent::KeyPress){
      m_qActiveKeyHash.insert(rec->keyId,*rec);
    }else if(rec->type == QEvent::KeyRelease){
      m_qActiveKeyHash.remove(rec->keyId);
    }
    emit(incomingData(rec->data()));
    delete rec;
    return true;
  }
  return false;
}
void MixxxKeyboard::stop() {
    m_stop = 1;
}

KbdController::KbdController():
Controller()
{
 
  setDeviceName("MixxxKeyboard");
  setDeviceCategory("keyboard");
  setOutputDevice(false);
  setInputDevice(true);
  m_pReader = KbdController::getKeyboard();
  setOpen(true);
  startEngine();
  connect(m_pReader, SIGNAL(incomingData(QByteArray)),
          this, SLOT(receive(QByteArray)));

}

KbdController::~KbdController() {
    close();
}
QString KbdController::presetExtension() {
    return KBD_PRESET_EXTENSION;
}
void KbdController::visit(const MidiControllerPreset* preset) {
    Q_UNUSED(preset);
    // TODO(XXX): throw a hissy fit.
    qDebug() << "ERROR: Attempting to load a MidiControllerPreset to a KbdController!";
}
void KbdController::visit(const HidControllerPreset* preset) {
    m_preset = *preset;
    // Emit presetLoaded with a clone of the preset.
    emit(presetLoaded(getPreset()));
}
bool KbdController::savePreset(const QString fileName) const {
    HidControllerPresetFileHandler handler;
    return handler.save(m_preset, getName(), fileName);
}
bool KbdController::matchPreset(const PresetInfo& preset) {
  const QList<QHash<QString,QString> > products = preset.getProducts();
  QHash <QString,QString>product;
  foreach(product,products){
    if(matchProductInfo(product))return true;
  }
  return false;
}
bool KbdController::matchProductInfo(QHash<QString,QString>info){
  return (info["protocol"]=="MixxxKeyboard");
}
int KbdController::open() {
    if (isOpen()) {
        qDebug() << "Kbd device" << getName() << "already open";
        return -1;
    }
    setOpen(true);
    startEngine();
    if (m_pReader == NULL) {
        m_pReader = KbdController::getKeyboard();
        qWarning() << "KbdReader not present for" << getName();
    } 
    connect(m_pReader, SIGNAL(incomingData(QByteArray)),
          this, SLOT(receive(QByteArray)));
    return 0;
}

int KbdController::close() {
    if (!isOpen()) {
        qDebug() << " device" << getName() << "already closed";
        return -1;
    }

    qDebug() << "Shutting down keyboard device" << getName();
    disconnect(m_pReader, SIGNAL(incomingData(QByteArray)),
                   this, SLOT(receive(QByteArray)));
    if (debugging()) qDebug() << "  Waiting on reader to finish";
    // Stop controller engine here to ensure it's done before the device is
    // closed incase it has any final parting messages
    stopEngine();
    // Close device
    if (debugging()) {
        qDebug() << "  Closing device";
    }
    setOpen(false);
    return 0;
}
MixxxKeyboard *KbdController::s_pKeyboard = NULL;
MixxxKeyboard *KbdController::getKeyboard(){
  if( s_pKeyboard==NULL)
    s_pKeyboard=new MixxxKeyboard();
  return s_pKeyboard;
}
void KbdController::send(QList<int> data, unsigned int length) {
    Q_UNUSED(length);Q_UNUSED(data);
}

void KbdController::send(QByteArray data) {
  Q_UNUSED(data);
}
