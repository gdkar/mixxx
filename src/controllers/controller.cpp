/**
* @file controller.h
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Sat Apr 30 2011
* @brief Base class representing a physical (or software) controller.
*/

#include <QApplication>
#include <QJSValue>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QtQml>

#include "controllers/controller.h"
#include "controllers/controllerdebug.h"
#include "controllers/defs_controllers.h"

Controller::Controller(QObject *p)
        : QObject(p),
//          m_pEngine(nullptr),
          m_bIsOutputDevice(false),
          m_bIsInputDevice(false),
          m_bIsOpen(false),
          m_bLearning(false) {
}

Controller::~Controller() = default;
    // Don't close the device here. Sub-classes should close the device in their
    // destructors.
bool Controller::isOpen() const
{
    return m_bIsOpen;
}
bool Controller::isOutputDevice() const
{
    return m_bIsOutputDevice;
}
bool Controller::isInputDevice() const
{
    return m_bIsInputDevice;
}
QString Controller::getDeviceName() const
{
    return m_sDeviceName;
    }
QString Controller::getDeviceCategory() const
{
    return m_sDeviceCategory;
}
bool Controller::isMappable() const
{
    return false;
}
bool Controller::isLearning() const
{
    return m_bLearning;
}
/*ControllerEngine* Controller::getEngine() const
{
    return m_pEngine;
}*/
void Controller::setDeviceName(QString deviceName)
{
    if(m_sDeviceName != deviceName)
        deviceNameChanged(m_sDeviceName = deviceName);
}
void Controller::setDeviceCategory(QString deviceCategory)
{
    if(getDeviceCategory() != deviceCategory)
        deviceCategoryChanged(m_sDeviceCategory = deviceCategory);
}
void Controller::setOutputDevice(bool outputDevice)
{
    if(outputDevice != isOutputDevice())
        isOutputDeviceChanged(m_bIsOutputDevice = outputDevice);
}
void Controller::setInputDevice(bool inputDevice)
{
    if(inputDevice != isInputDevice())
        isInputDeviceChanged(m_bIsInputDevice = inputDevice);
}
void Controller::setOpen(bool open)
{
    if(isOpen()!=open)
        isOpenChanged(m_bIsOpen = open);
}
int Controller::open()
{
    return -1;
}
int Controller::close()
{
    setOpen(false);
    return -1;
}
// Requests that the device poll if it is a polling device. Returns true
// if events were handled.
bool Controller::poll()
{
    return false;
}
void Controller::send(QByteArray data)
{
    void(sizeof(data));
}
// Returns true if this device should receive polling signals via calls to
// its poll() method.
bool Controller::isPolling() const
{
    return false;
}
void Controller::startLearning()
{
    if(!m_bLearning) {
        qDebug() << m_sDeviceName << "started learning";
        learningChanged(m_bLearning = true);
    }
}
void Controller::stopLearning()
{
    if(m_bLearning) {
        learningChanged(m_bLearning = false);
    }
}
void Controller::setLearning(bool l)
{
    if(l)
        startLearning();
    else
        stopLearning();
}
void Controller::send(QList<int> data, unsigned int length)
{
    // If you change this implementation, also change it in HidController (That
    // function is required due to HID devices having report IDs)
    QByteArray msg(length, 0);
    for (auto i = 0u; i < length; ++i)
        msg[i] = data.at(i);
    send(msg);
}
void Controller::receive(QVariant data, mixxx::Duration timestamp)
{
    return;
}
QString Controller::presetExtension() const
{
    return QString{};
}
BindProxy *Controller::getBindingFor(QString prefix)
{
    if(auto b = m_dispatch.value(prefix, nullptr)) {
        return b;
    }
    auto b = new BindProxy(prefix, this);
    m_dispatch.insert(prefix, b);
    return b;
}
