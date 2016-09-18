/**
* @file controllerengine.h
* @author Sean M. Pappalardo spappalardo@mixxx.org
* @date Sat Apr 30 2011
* @brief The script engine for use by a Controller.
*/

#ifndef CONTROLLERENGINE_H
#define CONTROLLERENGINE_H

#include <QTimerEvent>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QtQml>
#include <QtQuick>
#include <QQmlEngine>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueList>
#include <QJSValueIterator>

#include "preferences/usersettings.h"
#include "controllers/controllerpreset.h"
#include "controllers/softtakeover.h"
#include "util/alphabetafilter.h"
#include "util/duration.h"

// Forward declaration(s)
class Controller;
class ControlObjectScript;
class ControlProxy;
class ControllerEngine;

class ControllerEngine : public QObject {
    Q_OBJECT
  public:
    ControllerEngine(Controller* controller);
    virtual ~ControllerEngine();

    bool isReady();

    // Check whether a source file that was evaluated()'d has errors.
    bool hasErrors(QString filename);

    // Get the errors for a source file that was evaluated()'d
    const QStringList getErrors(QString filename);

    void setPopups(bool bPopups) {
        m_bPopups = bPopups;
    }
    // Wrap a snippet of JS code in an anonymous function
    QJSValue wrapFunctionCode(QString codeSnippet, int numberOfArgs);
    // Look up registered script function prefixes
    QStringList getScriptFunctionPrefixes() { return m_scriptFunctionPrefixes; };
    // Disconnect a ControllerEngineConnection
    template<class T>
    QJSValue toScriptValue(T&& val)
    {
        if(isReady()){
            return m_pEngine->toScriptValue(std::forward<T>(val));
        }else{
            return QJSValue{};
        }
    }
    QJSValue findObject(QString name);
  protected:
    Q_INVOKABLE double getValue(QString group, QString name);
    Q_INVOKABLE void setValue(QString group, QString name, double newValue);
    Q_INVOKABLE double getParameter(QString group, QString name);
    Q_INVOKABLE void setParameter(QString group, QString name, double newValue);
    Q_INVOKABLE double getParameterForValue(QString group, QString name, double value);
    Q_INVOKABLE void reset(QString group, QString name);
    Q_INVOKABLE double getDefaultValue(QString group, QString name);
    Q_INVOKABLE double getDefaultParameter(QString group, QString name);
    Q_INVOKABLE QJSValue connectControl(QString group, QString name,
                                            QJSValue function, bool disconnect = false);
    Q_INVOKABLE QJSValue connectControl(QString group, QString name,
                                            QJSValue function,
                                            QJSValue context,
                                            bool disconnect = false);

    // Called indirectly by the objects returned by connectControl
    Q_INVOKABLE void trigger(QString group, QString name);
    Q_INVOKABLE void log(QString message);
    Q_INVOKABLE int  beginTimer(int interval, QJSValue scriptCode,QJSValue context, bool oneShot = false);
    Q_INVOKABLE int  beginTimer(int interval, QJSValue scriptCode, bool oneShot = false);
    Q_INVOKABLE void stopTimer(int timerId);
    Q_INVOKABLE void scratchEnable(int deck, int intervalsPerRev, double rpm,
                                   double alpha, double beta, bool ramp = true);
    Q_INVOKABLE void scratchTick(int deck, int interval);
    Q_INVOKABLE void scratchDisable(int deck, bool ramp = true);
    Q_INVOKABLE bool isScratching(int deck);
    Q_INVOKABLE void softTakeover(QString group, QString name, bool set);
    Q_INVOKABLE void softTakeoverIgnoreNextValue(QString group, QString name);
    Q_INVOKABLE void brake(int deck, bool activate, double factor=0.9, double rate=1.0);
    Q_INVOKABLE void spinback(int deck, bool activate, double factor=1.8, double rate=-10.0);
  public slots:
    virtual void receive(QJSValueList args, mixxx::Duration timestamp = mixxx::Duration{});
     // Handler for timers that scripts set.
    virtual void timerEvent(QTimerEvent *event);

    // Evaluate a script file
    QJSValue evaluate(QString filepath);

    // Execute a basic MIDI message callback.
    bool execute(QJSValue function,
                 unsigned char channel,
                 unsigned char control,
                 unsigned char value,
                 unsigned char status,
                 QString group,
                 mixxx::Duration timestamp);
    QJSValue newArray(uint32_t length);
    QJSValue newObject();
    QJSValue newQObject(QObject *p);
    // Execute a byte array callback.
    bool execute(QJSValue function, QByteArray data,mixxx::Duration timestamp);

    // Evaluates all provided script files and returns true if no script errors
    // occurred while evaluating them.
    bool loadScriptFiles(QStringList scriptPaths,
                         const QList<ControllerPreset::ScriptFileInfo>& scripts);
    void initializeScripts();
    void gracefulShutdown();
    void scriptHasChanged(QString);
    ControlObjectScript* getControlObjectScript(QString group, QString name);
    Q_INVOKABLE QJSValue getControl(QString, QString);
  signals:
    void initialized();
    void resetController();

  private slots:
    void errorDialogButton(QString key, QMessageBox::StandardButton button);

  private:
    bool syntaxIsValid(QString scriptCode);
    QJSValue evaluate(QString scriptName, QStringList scriptPaths);
    bool internalExecute(QJSValue thisObject, QString scriptCode);
    bool internalExecute(QJSValue thisObject, QJSValue functionObject,
                         QJSValueList arguments);
    void initializeScriptEngine();

    void scriptErrorDialog(QString detailedError);
    void generateScriptFunctions(QString code);
    // Stops and removes all timers (for shutdown).
    void stopAllTimers();

    void callFunctionOnObjects(QStringList, QString, QJSValueList args = QJSValueList());
    bool checkException(QJSValue);
    QQmlEngine *m_pEngine;

    // Scratching functions & variables
    void scratchProcess(int timerId);

    bool isDeckPlaying(QString group);
    double getDeckRate(QString group);

    Controller* m_pController;
    bool m_bPopups;
    QMultiHash<ConfigKey, ControlObjectScript*> m_connectedControls;
    QStringList m_scriptFunctionPrefixes;
    QMap<QString, QStringList> m_scriptErrors;
    QHash<ConfigKey, ControlObjectScript*> m_controlCache;
    struct TimerInfo {
        QJSValue callback;
        QJSValue context;
        bool oneShot;
    };
    QHash<int, TimerInfo> m_timers;
    SoftTakeoverCtrl m_st;
    // 256 (default) available virtual decks is enough I would think.
    //  If more are needed at run-time, these will move to the heap automatically
    QVarLengthArray<int> m_intervalAccumulator;
    QVarLengthArray<mixxx::Duration> m_lastMovement;
    QVarLengthArray<double> m_dx, m_rampTo, m_rampFactor;
    QVarLengthArray<bool> m_ramp, m_brakeActive;
    QVarLengthArray<AlphaBetaFilter*> m_scratchFilters;
    QHash<int, int> m_scratchTimers;
    QJSValue                             m_globalObject;
    QHash<QString, QJSValue>             m_scriptWrappedFunctionCache;
    QHash<QString, QJSValue>             m_scriptModules;
    QList<QJSValue>                      m_scriptObjects;
    QList<std::pair<QJSValue,QJSValue> > m_receiveCallbacks;
    // Filesystem watcher for script auto-reload
    QFileSystemWatcher m_scriptWatcher;
    QStringList m_lastScriptPaths;

    friend class ControllerEngineTest;
};
#endif
