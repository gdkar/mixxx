/**
 * @file kbdenumerator.cpp
 * @author Neale Picket  neale@woozle.org
 * @date Thu Jun 28 2012
 * @brief USB Kbd controller backend
 */

#include <libusb.h>

#include "controllers/kbd/kbdcontroller.h"
#include "controllers/kbd/kbdenumerator.h"

KbdEnumerator::KbdEnumerator()
        : ControllerEnumerator() {
      (void)KbdController::getKeyboard();
}

KbdEnumerator::~KbdEnumerator() {
    qDebug() << "Deleting keyboard devices...";
    while (m_devices.size() > 0) {
        delete m_devices.takeLast();
    }
}
QList<Controller*> KbdEnumerator::queryDevices() {
    qDebug() << "Scanning USB Kbd devices:";
    if(m_devices.isEmpty()){
      KbdController *currentDevice = new KbdController();
      m_devices.push_back(currentDevice);
    }
    return m_devices;
}
