/**
  * @file controllermanager.h
  * @author Sean Pappalardo spappalardo@mixxx.org
  * @date Sat Apr 30 2011
  * @brief Manages creation/enumeration/deletion of hardware controllers.
  */

#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include <QSharedPointer>

#include "controllers/qmlcontrollerenumerator.h"
#include "preferences/usersettings.h"

//Forward declaration(s)
class Controller;
class ComponentEnumerator;
// Function to sort controllers by name
bool controllerCompare(Controller *a, Controller *b);

/** Manages enumeration/operation/deletion of hardware controllers. */
class ControllerManager : public QObject {
    Q_OBJECT
  public:
    ControllerManager(UserSettingsPointer pConfig);
    virtual ~ControllerManager();

    QList<Controller*> getControllers() const;
    QList<Controller*> getControllerList(bool outputDevices=true, bool inputDevices=true);

    // Prevent other parts of Mixxx from having to manually connect to our slots
    void setUpDevices() { emit(requestSetUpDevices()); };

    static QStringList getScriptPaths(UserSettingsPointer pConfig);

    // If pathOrFilename is an absolute path, returns it. If it is a relative
    // path and it is contained within any of the directories in presetPaths,
    // returns the path to the first file in the path that exists.
    static QString getAbsolutePath(QString pathOrFilename,QStringList presetPaths);

    bool importScript(QString scriptPath, QString* newScriptFileName);
    static bool checksumFile(QString filename, quint16* pChecksum);

  signals:
    void devicesChanged();
    void requestSetUpDevices();
    void requestShutdown();
    void requestSave(bool onlyActive);
    void requestInitialize();

  public slots:
    void updateControllerList();

    void openController(Controller* pController);
    void closeController(Controller* pController);

  private slots:
    // Perform initialization that should be delayed until the ControllerManager
    // thread is started.
    void onInitialize();
    // Open whatever controllers are selected in the preferences. This currently
    // only runs on start-up but maybe should instead be signaled by the
    // preferences dialog on apply, and only open/close changed devices
    void onSetUpDevices();
    void onShutdown();
    // Calls poll() on all devices that have isPolling() true.
    void pollDevices();
    void startPolling();
    void stopPolling();
    void maybeStartOrStopPolling();

    static QString presetFilenameFromName(QString name) {
        return name.replace(" ", "_").replace("/", "_").replace("\\", "_");
    }

  private:
    UserSettingsPointer m_pConfig;
    QTimer m_pollTimer;
    mutable QMutex m_mutex;
    QList<Controller*> m_controllers;
    QmlControllerEnumerator *m_qmlEnumerator{};
    QMap<Controller*, QQmlContext*> m_deviceContexts{};
    QMap<Controller*, QObject*>     m_deviceObjects{};
    QList<QQmlComponent *> m_components{};
    QThread* m_pThread;
    QQmlEngine    *m_pQmlEngine;
    QQmlContext   *m_mainContext{};
    QQmlComponent *m_mainComponent{};
    QObject       *m_mainInstance{};
    bool m_skipPoll;
};

#endif  // CONTROLLERMANAGER_H
