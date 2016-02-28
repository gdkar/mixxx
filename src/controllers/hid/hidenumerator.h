/**
* @file hidenumerator.h
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Sat Apr 30 2011
* @brief This class handles discovery and enumeration of DJ controllers that use the USB-HID protocol
*/

#include "controllers/controllerenumerator.h"

class HidEnumerator : public ControllerEnumerator {
  Q_OBJECT
  public:
    HidEnumerator();
    virtual ~HidEnumerator();
    QList<Controller*> queryDevices();
  private:
    QList<Controller*> m_devices;
};
