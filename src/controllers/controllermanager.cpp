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
#include "util/cmdlineargs.h"
#include "util/time.h"
#include "controllers/midi/portmidicontroller.h"
#include "controllers/midi/rtmidicontroller.h"
#include "controllers/keyboard/keyboardeventfilter.h"
#include "controllers/keyproxy.h"
#include "controllers/bindproxy.h"


#ifdef __HID__
#include "controllers/hid/hidcontroller.h"
#endif

#ifdef __BULK__
#include "controllers/bulk/bulkcontroller.h"
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
          // ControllerManager because the CM is moved to its own thread and runs
          // its own event loop.
          m_pollTimer(this),
          m_skipPoll(false)
{
    qmlRegisterUncreatableType<ConfigKey>("org.mixxx.qml", 0, 1, "ConfigKey","you can't make one of those....");
    qmlRegisterType<KeyProxy>("org.mixxx.qml", 0, 1, "KeyProxy");
    qmlRegisterType<BindProxy>("org.mixxx.qml", 0, 1, "BindProxy");
    qmlRegisterType<Controller>("org.mixxx.qml", 0, 1, "Controller");
    qmlRegisterUncreatableType<KeyboardEventFilter>("org.mixxx.qml", 0, 1, "KbdController",
        "There is only one static keyboard: just use that one. <_<");
    qmlRegisterUncreatableType<MidiController>("org.mixxx.qml",0,1,"MidiController",
        "MidiController is abstract, use RtMidiController or PortMidiController");
    qmlRegisterType<RtMidiController>("org.mixxx.qml", 0, 1, "RtMidiController");
    qmlRegisterType<PortMidiController>("org.mixxx.qml", 0, 1, "PortMidiController");
#ifdef __HID__
    qmlRegisterType<HidController>("org.mixxx.qml", 0, 1, "HidController");
#endif
#ifdef __BULK__
    qmlRegisterType<BulkController>("org.mixxx.qml", 0, 1, "BulkController");
#endif
    qmlRegisterType<ControlProxy>("org.mixxx.qml", 0, 1, "ControlProxy");
    qmlRegisterType<ControlObjectScript>("org.mixxx.qml", 0, 1, "ControlObjectScript");


    // Create controller mapping paths in the user's home directory.
    auto userPresets = userPresetsPath(m_pConfig);
    if (!QDir(userPresets).exists()) {
        qDebug() << "Creating user controller presets directory:" << userPresets;
        QDir().mkpath(userPresets);
    }
    //    m_pThread = new QThread{};
//    m_pThread->setObjectName("Controller");
    // Moves all children (including the poll timer) to m_pThread
//    moveToThread(m_pThread);
    // Controller processing needs to be prioritized since it can affect the
    // audio directly, like when scratching
//    m_pThread->start(QThread::HighPriority);
    m_pollTimer.setInterval(kPollIntervalMillis);
    m_pollTimer.setTimerType(Qt::PreciseTimer);
//    connect(&m_pollTimer, &QTimer::timeout,this, &ControllerManager::pollDevices);

    connect(this, &ControllerManager::requestInitialize,   this, &ControllerManager::onInitialize);
    connect(this, &ControllerManager::requestSetUpDevices, this, &ControllerManager::onSetUpDevices);
    connect(this, &ControllerManager::requestShutdown,     this, &ControllerManager::onShutdown);
    // Signal that we should run slotInitialize once our event loop has started
    // up.
    emit(requestInitialize());
}

ControllerManager::~ControllerManager()
{
    requestShutdown();
//    m_pThread->wait();
//    delete m_pThread;
}
void ControllerManager::onInitialize()
{
    qDebug() << "ControllerManager:onInitialize";
    // Initialize preset info parsers. This object is only for use in the main
    // thread. Do not touch it from within ControllerManager.
    auto presetSearchPaths = QStringList{}
        << userPresetsPath(m_pConfig)
        << resourcePresetsPath(m_pConfig);

    auto qmlSearchPaths = presetSearchPaths << resourceQmlPath(m_pConfig);
    {
//        auto paths = QStringList{} << userPresetsPath(m_pConfig)
//                                   << resourcePresetsPath(m_pConfig);
        auto qmlEnumerator = new QmlControllerEnumerator(this);
        connect(qmlEnumerator, &QmlControllerEnumerator::fileChanged, this, &ControllerManager::onFileChanged);
        qmlEnumerator->setSearchPaths(qmlSearchPaths);
        qmlEnumerator->refreshFileLists();
    }

    // Instantiate all enumerators. Enumerators can take a long time to
    // construct since they interact with host MIDI APIs.
    m_pQmlEngine = new QQmlEngine(this);
    for(auto && path : qmlSearchPaths) {
        m_pQmlEngine->addImportPath(path);
    }
    m_pQmlEngine->installExtensions(QQmlEngine::AllExtensions);

    QQmlEngine::setObjectOwnership(KeyboardEventFilter::create(), QQmlEngine::CppOwnership);
    auto keyboard = KeyboardEventFilter::instance();
    m_pQmlEngine->rootContext()->setContextProperty("Keyboard", keyboard);
    keyboard->setKeyboardHandler(this);

    auto mainPath = getAbsolutePath("main.qml", qmlSearchPaths);
//    m_scriptWatcher.addPath(mainPath);
    {
        QQmlComponent component(m_pQmlEngine);
        component.loadUrl(QUrl::fromLocalFile(mainPath));
        if(!component.isReady()) {
            qWarning() << component.errors();
            return;
        }
        m_mainInstance = component.create( );
    }
}
void ControllerManager::onFileChanged(QString path)
{
    auto presetSearchPaths = QStringList{}
        << userPresetsPath(m_pConfig)
        << resourcePresetsPath(m_pConfig);

    auto qmlSearchPaths = presetSearchPaths << resourceQmlPath(m_pConfig);

    auto mainPath = getAbsolutePath("main.qml", qmlSearchPaths);

    m_mainInstance->deleteLater();
    m_mainInstance = nullptr;
    m_pQmlEngine->clearComponentCache();
    auto keyboard = KeyboardEventFilter::instance();
    m_pQmlEngine->rootContext()->setContextProperty("Keyboard", keyboard);

//    m_scriptWatcher.addPath(mainPath);
    {
        QQmlComponent component(m_pQmlEngine);
        component.loadUrl(QUrl::fromLocalFile(mainPath));
        if(!component.isReady()) {
            qWarning() << component.errors();
            return;
        }
        m_mainInstance = component.create( );
    }
//    m_scriptWatcher.addPath(mainPath);

}
void ControllerManager::onShutdown()
{
    stopPolling();
            // Stop the processor after the enumerators since the engines live in it
//    m_pThread->quit();
}

void ControllerManager::updateControllerList()
{
    QMutexLocker locker(&m_mutex);

    QList<Controller*> newDeviceList;
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
        if (m_pConfig->getValueString(ConfigKey("[Controller]", name)) != "1") {
            continue;
        }
        // If we are in safe mode, skip opening controllers.
        if (CmdlineArgs::Instance().getSafeMode()) {
            qDebug() << "We are in safe mode -- skipping opening controller.";
            continue;
        }
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
}
void ControllerManager::closeController(Controller* pController)
{
    if (!pController)
        return;
}
// static
QStringList ControllerManager::getScriptPaths(UserSettingsPointer pConfig)
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
    *newScriptFileName = scriptFileName;
    return true;
}
