/**
  * @file controllermanager.h
  * @author Sean Pappalardo spappalardo@mixxx.org
  * @date Sat Apr 30 2011
  * @brief Manages creation/enumeration/deletion of hardware controllers.
  */
_Pragma("once")

#include <atomic>
#include <memory>
#include <QThread>
#include <QTimer>
#include <QString>
#include <QList>
#include <vector>

#include "preferences/usersettings.h"
#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetinfo.h"
#include "controllers/controllerpresetinfoenumerator.h"

//Forward declaration(s)
class Controller;
class ControllerEnumerator;
class EnumeratorFactory;
class ControllerLearningEventFilter;

// Function to sort controllers by name
bool controllerCompare(Controller *a,Controller *b);

/** Manages enumeration/operation/deletion of hardware controllers. */
class ControllerManager : public QObject
{
    Q_OBJECT
  public:
    ControllerManager(UserSettingsPointer pConfig);
    virtual ~ControllerManager();
    QList<Controller*> getControllers() const;
    QList<Controller*> getControllerList(bool outputDevices=true, bool inputDevices=true);
    ControllerLearningEventFilter* getControllerLearningEventFilter() const;
    PresetInfoEnumerator* getMainThreadPresetEnumerator();
    // Prevent other parts of Mixxx from having to manually connect to our slots
    void setUpDevices();
    void savePresets(bool onlyActive=false);
    static QList<QString> getPresetPaths(UserSettingsPointer pConfig);
    // If pathOrFilename is an absolute path, returns it. If it is a relative
    // path and it is contained within any of the directories in presetPaths,
    // returns the path to the first file in the path that exists.
    static QString getAbsolutePath(const QString& pathOrFilename,const QStringList& presetPaths);
    bool importScript(const QString& scriptPath, QString* newScriptFileName);
    static bool checksumFile(const QString& filename, quint16* pChecksum);
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
    void slotSetUpDevices();
    void slotShutdown();
    bool loadPreset(Controller* pController,ControllerPresetPointer preset);
    // Calls poll() on all devices that have isPolling() true.
    void pollDevices();
    void startPolling();
    void stopPolling();
    void maybeStartOrStopPolling();
    static QString presetFilenameFromName(QString name);
  private:
    UserSettingsPointer m_pConfig;
    std::unique_ptr<ControllerLearningEventFilter> m_pControllerLearningEventFilter;
    QTimer m_pollTimer;
    mutable QMutex m_mutex;
    std::vector<std::unique_ptr<ControllerEnumerator> > m_enumerators;
    QList<Controller*> m_controllers;
    std::unique_ptr<QThread> m_pThread;
    std::unique_ptr<PresetInfoEnumerator> m_pMainThreadPresetEnumerator;
    bool m_skipPoll{false};
    static std::atomic<EnumeratorFactory*> m_factories;
    friend class EnumeratorFactory;
};
