/**
* @file kbdenumerator.h
* @author Neale Pickett  neale@woozle.org
* @date Thu Jun 28 2012
* @brief Locate supported USB kbd controllers
*/

#ifndef KBDENUMERATOR_H
#define KBDENUMERATOR_H

#include "controllers/controllerenumerator.h"

struct libusb_context;

class KbdEnumerator : public ControllerEnumerator {
  public:
    KbdEnumerator();
    virtual ~KbdEnumerator();
    QList<Controller*> queryDevices();
  private:
    QList<Controller*> m_devices;
};

#endif
