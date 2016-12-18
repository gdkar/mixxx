/**
 * @file bulkenumerator.cpp
 * @author Neale Picket  neale@woozle.org
 * @date Thu Jun 28 2012
 * @brief USB Bulk controller backend
 */

#include <libusb.h>

#include "controllers/bulk/bulkcontroller.h"
#include "controllers/bulk/bulkenumerator.h"
#include "controllers/bulk/bulksupported.h"

BulkEnumerator::BulkEnumerator(QObject *p)
        : ControllerEnumerator(p),
          m_context(NULL)
{
    libusb_init(&m_context);
}

BulkEnumerator::~BulkEnumerator()
{
    qDebug() << "Deleting USB Bulk devices...";
    while (m_devices.size() > 0) {
        delete m_devices.takeLast();
    }
    libusb_exit(m_context);
}

static bool is_interesting(struct libusb_device_descriptor *desc)
{
    for (auto i = 0; bulk_supported[i].vendor_id; ++i) {
        if ((bulk_supported[i].vendor_id == desc->idVendor) &&
            (bulk_supported[i].product_id == desc->idProduct)) {
            return true;
        }
    }
    return false;
}

QList<Controller*> BulkEnumerator::queryDevices()
{
    qDebug() << "Scanning USB Bulk devices:";
    libusb_device **list;
    auto cnt = libusb_get_device_list(m_context, &list);
    auto err = 0;

    for (auto i = ssize_t{}; i < cnt; i++) {
        libusb_device *device = list[i];
        struct libusb_device_descriptor desc;

        libusb_get_device_descriptor(device, &desc);
        if (is_interesting(&desc)) {
            struct libusb_device_handle *handle = NULL;
            err = libusb_open(device, &handle);
            if (err) {
                qWarning() << "Error opening a device";
                continue;
            }

            auto currentDevice = new BulkController(m_context, handle, &desc);
            m_devices.push_back(currentDevice);
        }
    }
    libusb_free_device_list(list, 1);
    return m_devices;
}
