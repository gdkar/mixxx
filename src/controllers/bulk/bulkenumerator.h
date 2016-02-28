/**
* @file bulkenumerator.h
* @author Neale Pickett  neale@woozle.org
* @date Thu Jun 28 2012
* @brief Locate supported USB bulk controllers
*/

#include "controllers/controllerenumerator.h"

struct libusb_context;

class BulkEnumerator : public ControllerEnumerator {
    Q_OBJECT
  public:
    BulkEnumerator();
    virtual ~BulkEnumerator();

    QList<Controller*> queryDevices();

  private:
    QList<Controller*> m_devices;
    libusb_context* m_context;
};
