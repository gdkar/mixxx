/**
  * @file controllermanager.h
  * @author Sean Pappalardo spappalardo@mixxx.org
  * @date Sat Apr 30 2011
  * @brief Manages creation/enumeration/deletion of hardware controllers.
  */

#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include <QSharedPointer>

#include "controllers/controllerenumerator.h"
#include "controllers/qmlcontrollerenumerator.h"
#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetinfo.h"
#include "controllers/controllerpresetinfoenumerator.h"
#include "preferences/usersettings.h"

//Forward declaration(s)
class Controller;
class ControllerLearningEventFilter;
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
    ControllerLearningEventFilter* getControllerLearningEventFilter() const;
    QSharedPointer<PresetInfoEnumerator> getMainThreadPresetEnumerator() {
        return m_pMainThreadPresetEnumerator;
    }

    // Prevent other parts of Mixxx from having to manually connect to our slots
    void setUpDevices() { emit(requestSetUpDevices()); };
    void savePresets(bool onlyActive=false) { emit(requestSave(onlyActive)); };

    static QStringList getPresetPaths(UserSettingsPointer pConfig);

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

    // Writes out presets for currently connected input devices
    void onSavePresets(bool onlyActive=false);

  private slots:
    // Perform initialization that should be delayed until the ControllerManager
    // thread is started.
    void onInitialize();
    // Open whatever controllers are selected in the preferences. This currently
    // only runs on start-up but maybe should instead be signaled by the
    // preferences dialog on apply, and only open/close changed devices
    void onSetUpDevices();
    void onShutdown();
    bool loadPreset(Controller* pController,
                    ControllerPresetPointer preset);
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
    ControllerLearningEventFilter* m_pControllerLearningEventFilter;
    QTimer m_pollTimer;
    mutable QMutex m_mutex;
    QList<ControllerEnumerator*> m_enumerators;
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
    QSharedPointer<PresetInfoEnumerator> m_pMainThreadPresetEnumerator;
    bool m_skipPoll;
};

#endif  // CONTROLLERMANAGER_H
