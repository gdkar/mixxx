/**
  * @file controllermanager.cpp
  * @author Sean Pappalardo spappalardo@mixxx.org
  * @date Sat Apr 30 2011
  * @brief Manages creation/enumeration/deletion of hardware controllers.
  */

#include <QSet>
#include <QStringList>
#include "util/trace.h"
#include "control/controlobjectscript.h"
#include "control/controlproxy.h"
#include "control/controlobject.h"
#include "controllers/controllermanager.h"
#include "controllers/defs_controllers.h"
#include "controllers/controllerlearningeventfilter.h"
#include "util/cmdlineargs.h"
#include "util/time.h"
#include "controllers/midi/portmidicontroller.h"
#include "controllers/midi/rtmidicontroller.h"

#include "controllers/midi/portmidienumerator.h"
#include "controllers/midi/rtmidienumerator.h"
#ifdef __HSS1394__
#include "controllers/midi/hss1394enumerator.h"
#endif

#ifdef __HID__
#include "controllers/hid/hidenumerator.h"
#endif

#ifdef __BULK__
#include "controllers/bulk/bulkenumerator.h"
#endif

namespace {
// http://developer.qt.nokia.com/wiki/Threads_Events_QObjects

// Poll every 1ms (where possible) for good controller response
const int kPollIntervalMillis = 1;

} // anonymous namespace

QString firstAvailableFilename(QSet<QString>& filenames,QString originalFilename)
{
    auto filename = originalFilename;
    auto i = 1;
    while (filenames.contains(filename)) {
        i++;
        filename = QString("%1--%2").arg(originalFilename, QString::number(i));
    }
    filenames.insert(filename);
    return filename;
}
bool controllerCompare(Controller *a,Controller *b)
{
    return a->getDeviceName() < b->getDeviceName();
}
ControllerManager::ControllerManager(UserSettingsPointer pConfig)
        : QObject(),
          m_pConfig(pConfig),
          // WARNING: Do not parent m_pControllerLearningEventFilter to
          // ControllerManager because the CM is moved to its own thread and runs
          // its own event loop.
          m_pControllerLearningEventFilter(new ControllerLearningEventFilter()),
          m_pollTimer(this),
          m_skipPoll(false)
{
    qRegisterMetaType<ControllerPresetPointer>("ControllerPresetPointer");

    qmlRegisterType<BindingProxy>("org.mixxx.qml", 0, 1, "BindingProxy");
    qmlRegisterType<Controller>("org.mixxx.qml", 0, 1, "Controller");
    qmlRegisterUncreatableType<MidiController>("org.mixxx.qml",0,1,"MidiController",
        "MidiController is abstract, use RtMidiController or PortMidiController");
    qmlRegisterType<RtMidiController>("org.mixxx.qml", 0, 1, "RtMidiController");
    qmlRegisterType<PortMidiController>("org.mixxx.qml", 0, 1, "PortMidiController");
    qmlRegisterType<ControlProxy>("org.mixxx.qml", 0, 1, "ControlProxy");
    qmlRegisterType<ControlObjectScript>("org.mixxx.qml", 0, 1, "ControlObjectScript");

    // Create controller mapping paths in the user's home directory.
    auto userPresets = userPresetsPath(m_pConfig);
    if (!QDir(userPresets).exists()) {
        qDebug() << "Creating user controller presets directory:" << userPresets;
        QDir().mkpath(userPresets);
    }

    m_pThread = new QThread{};
    m_pThread->setObjectName("Controller");
    // Moves all children (including the poll timer) to m_pThread
    moveToThread(m_pThread);
    // Controller processing needs to be prioritized since it can affect the
    // audio directly, like when scratching
    m_pThread->start(QThread::HighPriority);
    m_pollTimer.setInterval(kPollIntervalMillis);
    m_pollTimer.setTimerType(Qt::PreciseTimer);
//    connect(&m_pollTimer, &QTimer::timeout,this, &ControllerManager::pollDevices);

    connect(this, &ControllerManager::requestInitialize,   this, &ControllerManager::onInitialize);
    connect(this, &ControllerManager::requestSetUpDevices, this, &ControllerManager::onSetUpDevices);
    connect(this, &ControllerManager::requestShutdown,     this, &ControllerManager::onShutdown);
    connect(this, &ControllerManager::requestSave,         this, &ControllerManager::onSavePresets);

    // Signal that we should run slotInitialize once our event loop has started
    // up.
    emit(requestInitialize());
}

ControllerManager::~ControllerManager()
{
    requestShutdown();
    m_pThread->wait();
    delete m_pThread;
    m_pControllerLearningEventFilter->deleteLater();
}

ControllerLearningEventFilter* ControllerManager::getControllerLearningEventFilter() const
{
    return m_pControllerLearningEventFilter;
}

void ControllerManager::onInitialize()
{
    qDebug() << "ControllerManager:onInitialize";
    // Initialize preset info parsers. This object is only for use in the main
    // thread. Do not touch it from within ControllerManager.
    auto presetSearchPaths = QStringList{}
        << userPresetsPath(m_pConfig)
        << resourcePresetsPath(m_pConfig);

    m_pMainThreadPresetEnumerator = QSharedPointer<PresetInfoEnumerator>::create(presetSearchPaths);
//    m_qmlEnumerator = new QmlControllerEnumerator(presetSearchPaths, this);

    auto qmlSearchPaths = presetSearchPaths << resourceQmlPath(m_pConfig);
    // Instantiate all enumerators. Enumerators can take a long time to
    // construct since they interact with host MIDI APIs.
    m_pQmlEngine = new QQmlEngine(this);
    for(auto && path : qmlSearchPaths) {
        m_pQmlEngine->addImportPath(path);
    }
    m_pQmlEngine->installExtensions(QQmlEngine::AllExtensions);
    m_mainContext = new QQmlContext(m_pQmlEngine->rootContext(),m_pQmlEngine);
    auto mainPath = getAbsolutePath("main.qml", qmlSearchPaths);
//    m_scriptWatcher.addPath(mainPath);
    m_mainComponent = new QQmlComponent(m_pQmlEngine,m_pQmlEngine);
    auto continueLoading = [this]()
        {
            if(m_mainComponent->isError()) {
                qWarning() << m_mainComponent->errors();
            } else if(m_mainComponent->isReady()) {
                if(!m_mainInstance)
                    m_mainInstance = m_mainComponent->create(m_mainContext);
            }
        };
    connect(m_mainComponent, &QQmlComponent::statusChanged, this, continueLoading);
    m_mainComponent->loadUrl(QUrl::fromLocalFile(mainPath));
}

void ControllerManager::onShutdown()
{
    stopPolling();
    // Clear m_enumerators before deleting the enumerators to prevent other code
    // paths from accessing them.
    {
        QMutexLocker locker(&m_mutex);
        m_enumerators.clear();
    }
    for(auto && e: findChildren<ControllerEnumerator*>())
        delete e;
        // Stop the processor after the enumerators since the engines live in it
    m_pThread->quit();
}

void ControllerManager::updateControllerList()
{
    QMutexLocker locker(&m_mutex);
    if (m_enumerators.isEmpty()) {
        qWarning() << "updateControllerList called but no enumerators have been added!";
        return;
    }
    auto enumerators = m_enumerators;
    locker.unlock();

    QList<Controller*> newDeviceList;
    foreach (ControllerEnumerator* pEnumerator, enumerators) {
        newDeviceList.append(pEnumerator->queryDevices());
    }

    locker.relock();
    if (newDeviceList != m_controllers) {
        m_controllers = newDeviceList;
        locker.unlock();
        emit(devicesChanged());
    }
}

QList<Controller*> ControllerManager::getControllers() const
{
    QMutexLocker locker(&m_mutex);
    return m_controllers;
}

QList<Controller*> ControllerManager::getControllerList(bool bOutputDevices, bool bInputDevices)
{
    qDebug() << "ControllerManager::getControllerList";
    auto controllers = getControllers();
    // Create a list of controllers filtered to match the given input/output
    // options.
    QList<Controller*> filteredDeviceList;

    for(auto && device: controllers) {
        if ((bOutputDevices == device->isOutputDevice()) ||
            (bInputDevices == device->isInputDevice())) {
            filteredDeviceList.push_back(device);
        }
    }
    return filteredDeviceList;
}

void ControllerManager::onSetUpDevices()
{
    qDebug() << "ControllerManager: Setting up devices";
    updateControllerList();
    auto deviceList = getControllerList(false, true);
    QSet<QString> filenames;
    for(auto && pController: deviceList) {
        auto name = pController->getDeviceName();
        if (pController->isOpen()) {
            pController->close();
        }
        // The filename for this device name.
        auto presetBaseName = presetFilenameFromName(name);
        // The first unique filename for this device (appends numbers at the end
        // if we have already seen a controller by this name on this run of
        // Mixxx.
        presetBaseName = firstAvailableFilename(filenames, presetBaseName);

        auto pPreset = ControllerPresetFileHandler::loadPreset(
                    presetBaseName + pController->presetExtension(),
                    getPresetPaths(m_pConfig));

        if (!loadPreset(pController, pPreset)) {
            // TODO(XXX) : auto load midi preset here.
            continue;
        }
        if (m_pConfig->getValueString(ConfigKey("[Controller]", presetBaseName)) != "1") {
            continue;
        }
        // If we are in safe mode, skip opening controllers.
        if (CmdlineArgs::Instance().getSafeMode()) {
            qDebug() << "We are in safe mode -- skipping opening controller.";
            continue;
        }
        qDebug() << "Opening controller:" << name;
        auto value = pController->open();
        if (value != 0) {
            qWarning() << "There was a problem opening" << name;
            continue;
        }
        pController->applyPreset(getPresetPaths(m_pConfig), true);
    }
    maybeStartOrStopPolling();
}
void ControllerManager::maybeStartOrStopPolling()
{
    QMutexLocker locker(&m_mutex);
    auto controllers = m_controllers;
    locker.unlock();
    auto shouldPoll = false;
    foreach (Controller* pController, controllers) {
        if (pController->isOpen() && pController->isPolling()) {
            shouldPoll = true;
        }
    }
    if (shouldPoll) {
        startPolling();
    } else {
        stopPolling();
    }
}
void ControllerManager::startPolling()
{
    // Start the polling timer.
    if (!m_pollTimer.isActive()) {
//        m_pollTimer.start();
        qDebug() << "Controller polling started.";
    }
}
void ControllerManager::stopPolling()
{
    m_pollTimer.stop();
    qDebug() << "Controller polling stopped.";
}
void ControllerManager::pollDevices() {
    // Note: this function is called from a high priority thread which
    // may stall the GUI or may reduce the available CPU time for other
    // High Priority threads like caching reader or broadcasting more
    // then desired, if it is called endless loop like.
    //
    // This especially happens if a controller like the 3x Speed
    // Stanton SCS.1D emits more massages than Mixxx is able to handle
    // or a controller like Hercules RMX2 goes wild. In such a case the
    // receive buffer is stacked up every call to insane values > 500 messages.
    //
    // To avoid this we pick here a strategies similar like the audio
    // thread. In case pollDevice() takes longer than a call cycle
    // we are cooperative a skip the next cycle to free at least some
    // CPU time
    //
    // Some random test data form a i5-3317U CPU @ 1.70GHz Running
    // Ubuntu Trusty:
    // * Idle poll: ~5 µs.
    // * 5 messages burst (full midi bandwidth): ~872 µs.

    if (m_skipPoll) {
        // skip poll in overload situation
        m_skipPoll = false;
        return;
    }
    auto start = mixxx::Time::elapsed();
    foreach (Controller* pDevice, m_controllers) {
        if (pDevice->isOpen() && pDevice->isPolling())
            pDevice->poll();
    }
    auto duration = mixxx::Time::elapsed() - start;
    if (duration > mixxx::Duration::fromMillis(kPollIntervalMillis))
        m_skipPoll = true;
    //qDebug() << "ControllerManager::pollDevices()" << duration << start;
}
void ControllerManager::openController(Controller* pController)
{
/*    if (!pController)
        return;
    if (pController->isOpen())
        pController->close();

    auto result = pController->open();
    maybeStartOrStopPolling();
    // If successfully opened the device, apply the preset and save the
    // preference setting.
    if (result == 0)
    {
        pController->applyPreset(getPresetPaths(m_pConfig), true);
        // Update configuration to reflect controller is enabled.
        m_pConfig->setValue(ConfigKey(
            "[Controller]", presetFilenameFromName(pController->getDeviceName())), 1);
    }*/
}
void ControllerManager::closeController(Controller* pController)
{
    if (!pController)
        return;
/*    pController->close();
    maybeStartOrStopPolling();
    // Update configuration to reflect controller is disabled.
    m_pConfig->setValue(ConfigKey(
        "[Controller]", presetFilenameFromName(pController->getDeviceName())), 0);*/
}
bool ControllerManager::loadPreset(Controller* pController,
                                   ControllerPresetPointer preset)
{
/*    if (!preset) {
        return false;
    }
    pController->setPreset(*preset.data());
    // Save the file path/name in the config so it can be auto-loaded at
    // startup next time
    m_pConfig->set(
        ConfigKey("[ControllerPreset]",presetFilenameFromName(pController->getDeviceName())),
        preset->filePath());*/
    return true;
}
void ControllerManager::onSavePresets(bool onlyActive)
{
/*    auto deviceList = getControllerList(false, true);
    QSet<QString> filenames;

    // TODO(rryan): This should be split up somehow but the filename selection
    // is dependent on all of the controllers to prevent over-writing each
    // other. We need a better solution.
    foreach (Controller* pController, deviceList) {
        if (onlyActive && !pController->isOpen()) {
            continue;
        }
        auto name = pController->getDeviceName();
        auto filename = firstAvailableFilename(
            filenames, presetFilenameFromName(name));
        auto presetPath = userPresetsPath(m_pConfig) + filename
                + pController->presetExtension();
        if (!pController->savePreset(presetPath)) {
            qWarning() << "Failed to write preset for device"
                       << name << "to" << presetPath;
        }
    }*/
}

// static
QStringList ControllerManager::getPresetPaths(UserSettingsPointer pConfig)
{
    auto scriptPaths = QStringList{};
    scriptPaths.append(userPresetsPath(pConfig));
    scriptPaths.append(resourcePresetsPath(pConfig));
    scriptPaths.append(resourceQmlPath(pConfig));
    return scriptPaths;
}
// static
bool ControllerManager::checksumFile(QString filename,quint16* pChecksum)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    auto fileSize = file.size();
    auto pFile = reinterpret_cast<const char*>(file.map(0, fileSize));
    if (pFile == NULL) {
        file.close();
        return false;
    }
    *pChecksum = qChecksum(pFile, fileSize);
    file.close();
    return true;
}

// static
QString ControllerManager::getAbsolutePath(QString pathOrFilename,
                                            QStringList paths)
{
    QFileInfo fileInfo(pathOrFilename);
    if (fileInfo.isAbsolute())
        return pathOrFilename;
    for(auto &&path: paths) {
        QDir pathDir(path);
        if (pathDir.exists(pathOrFilename)) {
            return pathDir.absoluteFilePath(pathOrFilename);
        }
    }
    return QString();
}

bool ControllerManager::importScript(QString scriptPath,QString* newScriptFileName)
{
    QDir userPresets(userPresetsPath(m_pConfig));
    qDebug() << "ControllerManager::importScript importing script" << scriptPath
             << "to" << userPresets.absolutePath();
    QFile scriptFile(scriptPath);
    QFileInfo script(scriptFile);
    if (!script.exists() || !script.isReadable()) {
        qWarning() << "ControllerManager::importScript script does not exist"
                   << "or is unreadable:" << scriptPath;
        return false;
    }
    // Not fatal if we can't checksum but still warn about it.
    quint16 scriptChecksum = 0;
    auto scriptChecksumGood = checksumFile(scriptPath, &scriptChecksum);
    if (!scriptChecksumGood) {
        qWarning() << "ControllerManager::importScript could not checksum file:" << scriptPath;
    }
    // The name we will save this file as in our local script repository. The
    // conflict resolution logic below will mutate this variable if the name is
    // already taken.
    auto scriptFileName = script.fileName();
    // For a file like "myfile.foo.bar.js", scriptBaseName is "myfile.foo.bar"
    // and scriptSuffix is "js".
    auto scriptBaseName = script.completeBaseName();
    auto scriptSuffix = script.suffix();
    auto conflictNumber = 1;
    // This script exists.
    while (userPresets.exists(scriptFileName)) {
        // If the two files are identical. We're done.
        quint16 localScriptChecksum = 0;
        if (checksumFile(userPresets.filePath(scriptFileName), &localScriptChecksum) &&
            scriptChecksumGood && scriptChecksum == localScriptChecksum) {
            *newScriptFileName = scriptFileName;
            qDebug() << "ControllerManager::importScript" << scriptFileName
                     << "had identical checksum to a file of the same name."
                     << "Skipping import.";
            return true;
        }
        // Otherwise, we need to rename the file to a non-conflicting
        // name. Insert a .X where X is a counter that we count up until we find
        // a filename that does not exist.
        scriptFileName = QString("%1.%2.%3").arg(
            scriptBaseName,
            QString::number(conflictNumber++),
            scriptSuffix);
    }
    auto destinationPath = userPresets.filePath(scriptFileName);
    if (!scriptFile.copy(destinationPath)) {
        qDebug() << "ControllerManager::importScript could not copy script to"
                 << "local preset path:" << destinationPath;
        return false;
    }
    *newScriptFileName = scriptFileName;
    return true;
}
