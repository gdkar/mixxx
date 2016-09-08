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
        m_pEngine->initializeScripts(pPreset->scripts);
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
void Controller::receive(const QByteArray data, mixxx::Duration timestamp)
{
    if (!m_pEngine) {
        //qWarning() << "Controller::receive called with no active engine!";
        // Don't complain, since this will always show after closing a device as
        //  queued signals flush out
        return;
    }
    auto length = data.size();
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
    for(auto function: m_pEngine->getScriptFunctionPrefixes()) {
        if (function == "")
            continue;
        function.append(".incomingData");
        auto incomingData = m_pEngine->wrapFunctionCode(function, 2);
        if (!m_pEngine->execute(incomingData, data, timestamp)) {
            qWarning() << "Controller: Invalid script function" << function;
        }
    }
}
