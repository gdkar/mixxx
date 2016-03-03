/**
* @file controller.h
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Sat Apr 30 2011
* @brief Base class representing a physical (or software) controller.
*/

#include <QApplication>
#include <QScriptValue>

#include "controllers/controller.h"
#include "controllers/controllerdebug.h"
#include "controllers/defs_controllers.h"

Controller::Controller() = default;

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
bool Controller::isLearning() const
{
    return m_bLearning;
}
Controller::~Controller() = default;

void Controller::startEngine()
{
    controllerDebug("  Starting engine");
    if (m_pEngine != NULL) {
        qWarning() << "Controller: Engine already exists! Restarting:";
        stopEngine();
    }
    m_pEngine = new ControllerEngine(this);
}

void Controller::stopEngine() {
    controllerDebug("  Shutting down engine");
    if (m_pEngine == NULL) {
        qWarning() << "Controller::stopEngine(): No engine exists!";
        return;
    }
    m_pEngine->gracefulShutdown();
    delete m_pEngine;
    m_pEngine = NULL;
}

bool Controller::applyPreset(QList<QString> scriptPaths, bool initializeScripts) {
    qDebug() << "Applying controller preset...";

    auto pPreset = preset();

    // Load the script code into the engine
    if (m_pEngine == NULL) {
        qWarning() << "Controller::applyPreset(): No engine exists!";
        return false;
    }

    if (pPreset->scripts.isEmpty()) {
        qWarning() << "No script functions available! Did the XML file(s) load successfully? See above for any errors.";
        return true;
    }

    auto success = m_pEngine->loadScriptFiles(scriptPaths, pPreset->scripts);
    if (initializeScripts) {
        m_pEngine->initializeScripts(pPreset->scripts);
    }
    return success;
}

void Controller::startLearning() {
    qDebug() << m_sDeviceName << "started learning";
    m_bLearning = true;
}
void Controller::stopLearning()
{
    //qDebug() << m_sDeviceName << "stopped learning.";
    m_bLearning = false;

}
void Controller::send(QList<int> data, unsigned int length)
{
    // If you change this implementation, also change it in HidController (That
    // function is required due to HID devices having report IDs)

    auto msg = QByteArray(length, 0);
    for (unsigned int i = 0; i < length; ++i) {
        msg[i] = data.at(i);
    }
    send(msg);
}

void Controller::receive(const QByteArray data, mixxx::Duration timestamp)
{
    if (m_pEngine == NULL) {
        //qWarning() << "Controller::receive called with no active engine!";
        // Don't complain, since this will always show after closing a device as
        //  queued signals flush out
        return;
    }

    int length = data.size();
    if (ControllerDebug::enabled()) {
        // Formatted packet display
        auto message = QString("%1: t:%2, %3 bytes:\n")
                .arg(m_sDeviceName).arg(timestamp.formatMillisWithUnit()).arg(length);
        for(auto i=0; i<length; i++) {
            QString spacer=" ";
            if ((i+1) % 4 == 0) spacer="  ";
            if ((i+1) % 16 == 0) spacer="\n";
            message += QString("%1%2")
                        .arg((unsigned char)(data.at(i)), 2, 16, QChar('0')).toUpper()
                        .arg(spacer);
        }
        controllerDebug(message);
    }

    for (auto function: m_pEngine->getScriptFunctionPrefixes()) {
        if (function == "") {
            continue;
        }
        function.append(".incomingData");
        auto incomingData = m_pEngine->resolveFunction(function);
        if (!m_pEngine->execute(incomingData, data, timestamp)) {
            qWarning() << "Controller: Invalid script function" << function;
        }
    }
}
ControllerEngine* Controller::getEngine() const
{
    return m_pEngine;
}
void Controller::setDeviceName(QString deviceName)
{
    m_sDeviceName = deviceName;
}
void Controller::setDeviceCategory(QString deviceCategory)
{
    m_sDeviceCategory = deviceCategory;
}
void Controller::setOutputDevice(bool outputDevice)
{
    m_bIsOutputDevice = outputDevice;
}
void Controller::setInputDevice(bool inputDevice)
{
    m_bIsInputDevice = inputDevice;
}
void Controller::setOpen(bool open)
{
    m_bIsOpen = open;
}
void Controller::setPreset(const ControllerPreset& preset) {
    // We don't know the specific type of the preset so we need to ask
    // the preset to call our visitor methods with its type.
    preset.accept(this);
}
void Controller::accept(ControllerVisitor* visitor)
{
    if(visitor)
        visitor->visit(this);
}
bool Controller::poll()
{
    return false;
}
