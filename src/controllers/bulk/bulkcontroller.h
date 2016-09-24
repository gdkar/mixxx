/**
  * @file bulkcontroller.h
  * @author Neale Picket  neale@woozle.org
  * @date Thu Jun 28 2012
  * @brief USB Bulk controller backend
  */

#ifndef BULKCONTROLLER_H
#define BULKCONTROLLER_H

#include <atomic>

#include "controllers/controller.h"
#include "util/duration.h"
struct libusb_device_handle;
struct libusb_context;
struct libusb_device_descriptor;

class BulkReader : public QThread {
    Q_OBJECT
  public:
    BulkReader(libusb_device_handle *handle, unsigned char in_epaddr);
    virtual ~BulkReader();

    void stop();

  signals:
    void incomingData(QByteArray data, mixxx::Duration timestamp);

  protected:
    void run();

  private:
    libusb_device_handle* m_phandle;
    std::atomic<bool> m_stop;
    unsigned char m_in_epaddr;
};

class BulkController : public Controller {
    Q_OBJECT
  public:
        Q_INVOKABLE BulkController(QObject *pParent=nullptr);
    BulkController(libusb_context* context, libusb_device_handle *handle,
                   struct libusb_device_descriptor *desc);
    virtual ~BulkController();

    QString presetExtension() const override;

    virtual bool matchProductInfo(const ProductInfo& product);

  protected:
    Q_INVOKABLE void send(QList<int> data, unsigned int length);

  private slots:
    int open();
    int close();
  private:
    // For devices which only support a single report, reportID must be set to
    // 0x0.
    void send(QByteArray data) override;
    bool isPolling() const override;
    // Returns a pointer to the currently loaded controller preset. For internal
    // use only.
    libusb_context* m_context;
    libusb_device_handle *m_phandle;

    // Local copies of things we need from desc

    unsigned short vendor_id;
    unsigned short product_id;
    unsigned char in_epaddr;
    unsigned char out_epaddr;
    QString manufacturer;
    QString product;

    QString m_sUID;
    BulkReader* m_pReader;
};

#endif
