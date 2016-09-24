/**
  * @file hidcontroller.h
  * @author Sean M. Pappalardo  spappalardo@mixxx.org
  * @date Sun May 1 2011
  * @brief HID controller backend
  */

#ifndef HIDCONTROLLER_H
#define HIDCONTROLLER_H

#include <hidapi.h>

#include <atomic>

#include "controllers/controller.h"
#include "util/duration.h"
class HidReader : public QThread {
    Q_OBJECT
  public:
    HidReader(hid_device* device);
    virtual ~HidReader();

    void stop();
  signals:
    void incomingData(QByteArray data, mixxx::Duration timestamp);
  protected:
    void run();
  private:
    hid_device* m_pHidDevice;
    std::atomic<bool> m_stop{false};
};

class HidController : public Controller {
    Q_OBJECT
  public:
    Q_INVOKABLE HidController(QObject *p = nullptr);
    HidController(const hid_device_info deviceInfo);
   ~HidController();

    QString presetExtension() const override;

    virtual bool matchProductInfo(const ProductInfo& product);
    virtual void guessDeviceCategory();

    static QString safeDecodeWideString(const wchar_t* pStr, size_t max_length);

  protected:
    Q_INVOKABLE void send(QList<int> data, unsigned int length, unsigned int reportID = 0);

  private slots:
    int open() override;
    int close() override;

  private:
    // For devices which only support a single report, reportID must be set to
    // 0x0.
    virtual void send(QByteArray data);
    virtual void send(QByteArray data, unsigned int reportID);

    bool isPolling() const override;
    // Returns a pointer to the currently loaded controller preset. For internal
    // use only.

    // Local copies of things we need from hid_device_info
    int hid_interface_number;
    unsigned short hid_vendor_id;
    unsigned short hid_product_id;
    unsigned short hid_usage_page;
    unsigned short hid_usage;
    char* hid_path;
    wchar_t* hid_serial_raw;
    QString hid_serial;
    QString hid_manufacturer;
    QString hid_product;

    QString m_sUID;
    hid_device* m_pHidDevice;
    HidReader* m_pReader;
};

#endif
