/**
  * @file hidcontroller.h
  * @author Sean M. Pappalardo  spappalardo@mixxx.org
  * @date Sun May 1 2011
  * @brief HID controller backend
  */

_Pragma("once")
#include <hidapi.h>

#include <atomic>

#include "controllers/controller.h"
#include "controllers/hid/hidcontrollerpreset.h"
#include "controllers/hid/hidcontrollerpresetfilehandler.h"

class HidReader : public QThread {
    Q_OBJECT
  public:
    HidReader(hid_device* device);
    virtual ~HidReader();
    virtual void stop();
  signals:
    void incomingData(QByteArray data);
  protected:
    virtual void run();
  private:
    hid_device* m_pHidDevice = nullptr;
    std::atomic<bool> m_stop{false};
};
class HidController : public Controller {
    Q_OBJECT
  public:
    HidController(const hid_device_info deviceInfo);
    virtual ~HidController();
    virtual QString presetExtension();
    virtual ControllerPresetPointer getPreset() const;
    virtual bool savePreset(const QString fileName) const;
    virtual void visit(const MidiControllerPreset* preset);
    virtual void visit(const HidControllerPreset* preset);
    virtual void accept(ControllerVisitor* visitor);
    virtual bool isMappable() const;
    virtual bool matchPreset(const PresetInfo& preset);
    virtual bool matchProductInfo(QHash <QString,QString >);
    virtual void guessDeviceCategory();
  protected:
    Q_INVOKABLE void send(QList<int> data, unsigned int length, unsigned int reportID = 0);
  private slots:
    int open();
    int close();
  private:
    // For devices which only support a single report, reportID must be set to
    // 0x0.
    virtual void send(QByteArray data);
    virtual void send(QByteArray data, unsigned int reportID);
    virtual bool isPolling() const;
    // Returns a pointer to the currently loaded controller preset. For internal
    // use only.
    virtual ControllerPreset* preset();
    // Local copies of things we need from hid_device_info
    int hid_interface_number = 0;
    unsigned short hid_vendor_id = 0;
    unsigned short hid_product_id = 0;
    unsigned short hid_usage_page = 0;
    unsigned short hid_usage = 0;
    QString  hid_path;
    wchar_t* hid_serial_raw = nullptr;
    QString  hid_serial;
    QString  hid_manufacturer;
    QString  hid_product;

    QString m_sUID;
    hid_device* m_pHidDevice = nullptr;
    HidReader* m_pReader = nullptr;
    HidControllerPreset m_preset ;
};
