/**
* @file controllerengine.h
* @author Sean M. Pappalardo spappalardo@mixxx.org
* @date Sat Apr 30 2011
* @brief The script engine for use by a Controller.
*/

_Pragma("once")
#include <QEvent>


#include <QJSEngine>
#include <QJSValue>
#include <QJSValueList>
#include <QQmlEngine>
#include <QQmlContext>
#include <QtQml>

#include <QMessageBox>
#include <QFileSystemWatcher>
#include <memory>
#include <vector>
#include "configobject.h"
#include "controllers/softtakeover.h"
#include "controllers/controllerpreset.h"

// Forward declaration(s)
class Controller;
class ControlObject;
class ControllerEngine;
class AlphaBetaFilter;
// ControllerEngineConnection class for closure-compatible engine.connectControl
class ControllerEngineConnection {
  public:
    ConfigKey key;
    QString id;
    QJSValue function;
    ControllerEngine *ce;
    QJSValue context;
};
class ControllerEngineConnectionScriptValue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ readId);
    Q_PROPERTY(QJSValue function READ getFunction);
    Q_PROPERTY(QJSValue context READ getContext);
    // We cannot expose ConfigKey directly since it's not a
    // QObject
    //Q_PROPERTY(ConfigKey key READ key)
    // There's little use in exposing the function...
    //Q_PROPERTY(QJSValue function READ function)
  public:
    ControllerEngineConnectionScriptValue(ControllerEngineConnection _conn)
    : conn(_conn)
    {}
    QString readId() const
    { 
      return conn.id; 
    }
    QJSValue getFunction() const
    {
      return conn.function;
    }
    QJSValue getContext() const
    {
      return conn.context;
    }
    Q_INVOKABLE void disconnect();
  private:
   ControllerEngineConnection conn;
};
/* comparison function for ControllerEngineConnection */
inline bool operator==(const ControllerEngineConnection &c1, const ControllerEngineConnection &c2)
{
    return c1.id == c2.id && c1.key.group == c2.key.group && c1.key.item == c2.key.item;
}
class ControllerEngine : public QObject
{
    Q_OBJECT
  public:
    ControllerEngine(Controller* controller, QObject *pParent = nullptr);
    virtual ~ControllerEngine();
    bool isReady();
    // Check whether a source file that was evaluated()'d has errors.
    bool hasErrors(QString filename);
    // Get the errors for a source file that was evaluated()'d
    const QStringList getErrors(QString filename);
    void setDebug(bool bDebug)
    {
        m_bDebug = bDebug;
    }

    void setPopups(bool bPopups) {
        m_bPopups = bPopups;
    }

    /** Resolve a function name to a QJStValue. */
    QJSValue resolveFunction(QString function, bool useCache) const;
    /** Look up registered script function prefixes */
    QList<QString>& getScriptFunctionPrefixes() { return m_scriptFunctionPrefixes; };
    /** Disconnect a ControllerEngineConnection */
    void disconnectControl(const ControllerEngineConnection conn);

  protected:
    Q_INVOKABLE double getValue(QString group, QString name);
    Q_INVOKABLE void setValue(QString group, QString name, double newValue);
    Q_INVOKABLE double getParameter(QString group, QString name);
    Q_INVOKABLE void setParameter(QString group, QString name, double newValue);
    Q_INVOKABLE double getParameterForValue(QString group, QString name, double value);
    Q_INVOKABLE void reset(QString group, QString name);
    Q_INVOKABLE double getDefaultValue(QString group, QString name);
    Q_INVOKABLE double getDefaultParameter(QString group, QString name);
    Q_INVOKABLE QJSValue connectControl(QString group, QString name,QJSValue function, QJSValue thisobj, bool disconnect = false);
    // Called indirectly by the objects returned by connectControl
    Q_INVOKABLE void trigger(QString group, QString name);
    Q_INVOKABLE void log(QString message);
    Q_INVOKABLE int beginTimer(int interval, QJSValue scriptCode, QJSValue thisObject, bool oneShot = false);
    Q_INVOKABLE void stopTimer(int timerId);
    Q_INVOKABLE void scratchEnable(int deck, int intervalsPerRev, double rpm,double alpha, double beta, bool ramp = true);
    Q_INVOKABLE void scratchTick(int deck, int interval);
    Q_INVOKABLE void scratchDisable(int deck, bool ramp = true);
    Q_INVOKABLE bool isScratching(int deck);
    Q_INVOKABLE void softTakeover(QString group, QString name, bool set);
    Q_INVOKABLE void brake(int deck, bool activate, double factor=0.9, double rate=1.0);
    Q_INVOKABLE void spinback(int deck, bool activate, double factor=1.8, double rate=-10.0);

    // Handler for timers that scripts set.
    virtual void timerEvent(QTimerEvent *event);

  public slots:
    void slotValueChanged(double value);
    // Evaluate a script file
    bool evaluate(QString filepath);
    // Execute a particular function
    // Execute a particular function with a list of arguments
    bool execute(QString function, QJSValue thisobj = QJSValue{}, QJSValueList args = QJSValueList{});
    bool execute(QJSValue function, QJSValue thisobj = QJSValue{}, QJSValueList args = QJSValueList{});
    // Execute a particular function with a data string (e.g. a device ID)
    bool execute(QString function, QJSValue thisobj, QString data);
    // Execute a particular function with a list of arguments
    bool execute(QString function, QJSValue thisobj, const QByteArray data);
    bool execute(QJSValue function, QJSValue thisobj, const QByteArray data);
    // Execute a particular function with a data buffer
    //TODO: redo this one
    //bool execute(QString function, const QByteArray data);
    void loadScriptFiles(QList<QString> scriptPaths,const QList<ControllerPreset::ScriptFileInfo>& scripts);
    void initializeScripts(const QList<ControllerPreset::ScriptFileInfo>& scripts);
    void gracefulShutdown();
    void scriptHasChanged(QString);

  signals:
    void initialized();
    void resetController();

  private slots:
    void errorDialogButton(QString key, QMessageBox::StandardButton button);

  private:
    bool evaluate(QString scriptName, QList<QString> scriptPaths);
    bool internalExecute(QString scriptCode);
    bool internalExecute(QJSValue thisObject, QString scriptCode);
    bool internalExecute(QJSValue  thisObject, QJSValue functionObject);
    void initializeScriptEngine();

    void scriptErrorDialog(QString detailedError);
    void generateScriptFunctions(QString code);
    // Stops and removes all timers (for shutdown).
    void stopAllTimers();

    void callFunctionOnObjects(QList<QString>, QString, QJSValueList args = QJSValueList ());
    bool checkException(QJSValue);
    ControlObject* getControlObject(QString group, QString name);
    // Scratching functions & variables
    void scratchProcess(int timerId);

    bool isDeckPlaying(const QString& group);
    double getDeckRate(const QString& group);

    QJSEngine *m_pEngine =  nullptr;
    Controller* m_pController = nullptr;
    bool m_bDebug  = false;
    bool m_bPopups = false;
    QMultiHash<ConfigKey, ControllerEngineConnection> m_connectedControls;
    QList<QString> m_scriptFunctionPrefixes;
    QMap<QString,QStringList> m_scriptErrors;
    QHash<ConfigKey, ControlObject*> m_controlCache;
    struct TimerInfo
    {
        QJSValue callback;
        QJSValue context;
        bool oneShot;
    };
    QHash<int, TimerInfo> m_timers;
    SoftTakeoverCtrl m_st;
    // 256 (default) available virtual decks is enough I would think.
    //  If more are needed at run-time, these will move to the heap automatically
    std::vector<int> m_intervalAccumulator;
    std::vector<uint> m_lastMovement;
    std::vector<double> m_dx, m_rampTo, m_rampFactor;
    std::vector<bool> m_ramp, m_brakeActive;
    std::vector<std::unique_ptr<AlphaBetaFilter> > m_scratchFilters;
    QHash<int, int> m_scratchTimers;
    mutable QHash<QString, QJSValue > m_scriptValueCache;
    // Filesystem watcher for script auto-reload
    QFileSystemWatcher m_scriptWatcher;
    QList<QString> m_lastScriptPaths;
};
