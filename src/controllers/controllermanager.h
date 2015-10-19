/**
  * @file controllermanager.h
  * @author Sean Pappalardo spappalardo@mixxx.org
  * @date Sat Apr 30 2011
  * @brief Manages creation/enumeration/deletion of hardware controllers.
  */

_Pragma("once")
#include "configobject.h"
#include "controllers/controllerenumerator.h"
#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetinfo.h"
#include "controllers/controllerpresetinfoenumerator.h"

//Forward declaration(s)
class Controller;
class ControllerLearningEventFilter;

// Function to sort controllers by name
bool controllerCompare(Controller *a,Controller *b);

/** Manages enumeration/operation/deletion of hardware controllers. */
class ControllerManager : public QObject {
    Q_OBJECT
  public:
    ControllerManager(ConfigObject<ConfigValue> * pConfig);
    virtual ~ControllerManager();
    QList<Controller*> getControllers() const;
    QList<Controller*> getControllerList(bool outputDevices=true, bool inputDevices=true);
    ControllerLearningEventFilter* getControllerLearningEventFilter() const;
    PresetInfoEnumerator* getMainThreadPresetEnumerator();
    // Prevent other parts of Mixxx from having to manually connect to our slots
    void setUpDevices() { emit(requestSetUpDevices()); };
    void savePresets(bool onlyActive=false) { emit(requestSave(onlyActive)); };
    static QList<QString> getPresetPaths(ConfigObject<ConfigValue>* pConfig);
    // If pathOrFilename is an absolute path, returns it. If it is a relative
    // path and it is contained within any of the directories in presetPaths,
    // returns the path to the first file in the path that exists.
    static QString getAbsolutePath(QString pathOrFilename,
                                   QStringList presetPaths);
    bool importScript(QString scriptPath, QString* newScriptFileName);
    static bool checksumFile(QString filename, quint16* pChecksum);
  signals:
    void devicesChanged();
    void requestSetUpDevices();
    void requestShutdown();
    void requestSave(bool onlyActive);
  public slots:
    void updateControllerList();
    void openController(Controller* pController);
    void closeController(Controller* pController);
    // Writes out presets for currently connected input devices
    void slotSavePresets(bool onlyActive=false);
  private slots:
    // Open whatever controllers are selected in the preferences. This currently
    // only runs on start-up but maybe should instead be signaled by the
    // preferences dialog on apply, and only open/close changed devices
    int slotSetUpDevices();
    void slotShutdown();
    bool loadPreset(Controller* pController, ControllerPresetPointer preset);
    // Calls poll() on all devices that have isPolling() true.
    void pollDevices();
    void startPolling();
    void stopPolling();
    void maybeStartOrStopPolling();
    static QString presetFilenameFromName(QString name);
  private:
    ConfigObject<ConfigValue> *m_pConfig;
    ControllerLearningEventFilter* m_pControllerLearningEventFilter;
    QTimer m_pollTimer;
    mutable QMutex m_mutex;
    QList<ControllerEnumerator*> m_enumerators;
    QList<Controller*> m_controllers;
    QThread* m_pThread;
    PresetInfoEnumerator* m_pMainThreadPresetEnumerator;
};
