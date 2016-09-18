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
          m_pEngine(nullptr),
          m_bIsOutputDevice(false),
          m_bIsInputDevice(false),
          m_bIsOpen(false),
          m_bLearning(false) {
}

Controller::~Controller() = default;
    // Don't close the device here. Sub-classes should close the device in their
    // destructors.
void Controller::accept(ControllerVisitor* visitor)
{
    if(visitor)
        visitor->visit(this);
}

bool Controller::matchPreset(const PresetInfo& preset)
{
    void(sizeof(preset));
    return false;
}
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
QString Controller::getName() const
{
    return m_sDeviceName;
    }
QString Controller::getCategory() const
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

ControllerEngine* Controller::getEngine() const
{
    return m_pEngine;
}
void Controller::setDeviceName(QString deviceName)
{
    if(m_sDeviceName != deviceName)
        deviceNameChanged(m_sDeviceName = deviceName);
}
void Controller::setDeviceCategory(QString deviceCategory)
{
    if(getCategory() != deviceCategory)
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
// Returns a pointer to the currently loaded controller preset. For internal
// use only.
ControllerPreset* Controller::preset()
{
    return nullptr;
}
void Controller::setPreset(const ControllerPreset& preset)
{
    // We don't know the specific type of the preset so we need to ask
    // the preset to call our visitor methods with its type.
    preset.accept(this);
}
void Controller::startEngine()
{
    controllerDebug("  Starting engine");
    if (m_pEngine) {
        qWarning() << "Controller: Engine already exists! Restarting:";
        stopEngine();
    }
    engineChanged(m_pEngine = new ControllerEngine(this));
}

void Controller::stopEngine()
{
    controllerDebug("  Shutting down engine");
    if (!m_pEngine) {
        qWarning() << "Controller::stopEngine(): No engine exists!";
        return;
    }
    m_pEngine->gracefulShutdown();
    delete m_pEngine;
    engineChanged(m_pEngine = nullptr);
}
bool Controller::applyPreset(QList<QString> scriptPaths, bool initializeScripts)
{
    qDebug() << "Applying controller preset...";
    auto pPreset = preset();
    // Load the script code into the engine
    if (!m_pEngine) {
        qWarning() << "Controller::applyPreset(): No engine exists!";
        return false;
    }
    if (pPreset->scripts.isEmpty()) {
        qWarning() << "No script functions available! Did the XML file(s) load successfully? See above for any errors.";
        return true;
    }
    auto success = m_pEngine->loadScriptFiles(scriptPaths, pPreset->scripts);
    if (initializeScripts) {
        m_pEngine->initializeScripts();
    }
    return success;
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
    if (!m_pEngine) {
        //qWarning() << "Controller::receive called with no active engine!";
        // Don't complain, since this will always show after closing a device as
        //  queued signals flush out
        return;
    }
/*    auto length = data.size();
    auto arg = m_pEngine->newArray();
    for(auto i = 0u; i < data.size(); ++i)
        arg.setProperty(i, int(data[i]));
*/
    auto args = QJSValueList{} << m_pEngine->toScriptValue(data);
    m_pEngine->receive(args, timestamp);
}
QString Controller::presetExtension() const
{
    return QString{};
}
bool Controller::savePreset(QString filename) const
{
    Q_UNUSED(filename);
    return false;
}
ControllerPresetPointer Controller::getPreset() const
{
    return ControllerPresetPointer{};
}
