/**
  * @file bulkcontroller.h
  * @author Neale Picket  neale@woozle.org
  * @date Thu Jun 28 2012
  * @brief USB Bulk controller backend
  */

_Pragma("once")
#include <atomic>

#include "controllers/controller.h"
#include "controllers/hid/hidcontrollerpreset.h"
#include "controllers/hid/hidcontrollerpresetfilehandler.h"

struct libusb_device_handle;
struct libusb_context;
struct libusb_device_descriptor;

class BulkReader : public QThread {
    Q_OBJECT
  public:
    BulkReader(libusb_device_handle *handle, unsigned char in_epaddr);
    virtual ~BulkReader();
    virtual void stop();
  signals:
    void incomingData(QByteArray data);
  protected:
    virtual void run();
  private:
    libusb_device_handle* m_phandle = nullptr;
    std::atomic<bool> m_stop{false};
    unsigned char m_in_epaddr;
};

class BulkController : public Controller {
    Q_OBJECT
  public:
    BulkController(libusb_context* context, libusb_device_handle *handle,
                   struct libusb_device_descriptor *desc);
    virtual ~BulkController();
    virtual QString presetExtension();
    virtual ControllerPresetPointer getPreset() const;

    virtual bool savePreset(const QString fileName) const;

    virtual void visit(const MidiControllerPreset* preset);
    virtual void visit(const HidControllerPreset* preset);

    virtual void accept(ControllerVisitor* visitor);
    virtual bool isMappable() const;
    virtual bool matchPreset(const PresetInfo& preset);
    virtual bool matchProductInfo(QHash <QString,QString >);
  protected:
    Q_INVOKABLE void send(QList<int> data, unsigned int length);
  private slots:
    int open();
    int close();
  private:
    // For devices which only support a single report, reportID must be set to
    // 0x0.
    virtual void send(QByteArray data);
    virtual bool isPolling() const;
    // Returns a pointer to the currently loaded controller preset. For internal
    // use only.
    virtual ControllerPreset* preset();
    libusb_context* m_context = nullptr;
    libusb_device_handle *m_phandle = nullptr;
    // Local copies of things we need from desc
    unsigned short vendor_id  = 0;
    unsigned short product_id = 0;
    unsigned char in_epaddr   = 0;
    unsigned char out_epaddr  = 0;
    QString manufacturer;
    QString product;
    QString m_sUID;
    BulkReader* m_pReader = nullptr;
    HidControllerPreset m_preset;
};
