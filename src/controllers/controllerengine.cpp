/***************************************************************************
                          controllerengine.cpp  -  description
                          -------------------
    begin                : Sat Apr 30 2011
    copyright            : (C) 2011 by Sean M. Pappalardo
    email                : spappalardo@mixxx.org
 ***************************************************************************/

#include "controllers/controllerengine.h"

#include "controllers/controller.h"
#include "controllers/controllerdebug.h"
#include "control/controlobject.h"
#include "control/controlobjectscript.h"
#include "errordialoghandler.h"
#include "mixer/playermanager.h"
// to tell the msvs compiler about `isnan`
#include "util/math.h"
#include "util/time.h"

// Used for id's inside controlConnection objects
// (closure compatible version of connectControl)
#include <QUuid>

const int kDecks = 16;

// Use 1ms for the Alpha-Beta dt. We're assuming the OS actually gives us a 1ms
// timer.
const int kScratchTimerMs = 1;
const double kAlphaBetaDt = kScratchTimerMs / 1000.0;

ControllerEngine::ControllerEngine(Controller* controller)
        : m_pEngine(nullptr),
          m_pController(controller),
          m_bPopups(false)
{
    // Handle error dialog buttons
    qRegisterMetaType<QMessageBox::StandardButton>("QMessageBox::StandardButton");

    // Pre-allocate arrays for average number of virtual decks
    m_intervalAccumulator.resize(kDecks);
    m_lastMovement.resize(kDecks);
    m_dx.resize(kDecks);
    m_rampTo.resize(kDecks);
    m_ramp.resize(kDecks);
    m_scratchFilters.resize(kDecks);
    m_rampFactor.resize(kDecks);
    m_brakeActive.resize(kDecks);
    // Initialize arrays used for testing and pointers
    for (int i = 0; i < kDecks; ++i) {
        m_dx[i] = 0.0;
        m_scratchFilters[i] = new AlphaBetaFilter(this);
        m_ramp[i] = false;
    }

    initializeScriptEngine();
}

ControllerEngine::~ControllerEngine()
{

    // Delete the script engine, first clearing the pointer so that
    // other threads will not get the dead pointer after we delete it.
    if(auto engine = std::exchange(m_pEngine,nullptr)) {
        engine->collectGarbage();
        engine->deleteLater();
    }
}

/* -------- ------------------------------------------------------
Purpose: Calls the same method on a list of JS Objects
Input:   -
Output:  -
-------- ------------------------------------------------------ */
void ControllerEngine::callFunctionOnObjects(QStringList scriptFunctionPrefixes,
                                             QString function, QJSValueList args)
{
    auto global = m_pEngine->globalObject();

    for (auto && prefixName : scriptFunctionPrefixes) {
        auto prefix = global.property(prefixName);
        if (prefix.isError() || prefix.isUndefined() || !prefix.isObject()) {
            qWarning() << "ControllerEngine: No" << prefixName << "object in script";
            continue;
        }

        auto init = prefix.property(function);
        if (!init.isCallable()) {
            qWarning() << "ControllerEngine:" << prefixName << "has no" << function << " method";
            continue;
        }
        controllerDebug("ControllerEngine: Executing" << prefixName << "." << function);
        init.callWithInstance(prefix, args);
    }
}

/* ------------------------------------------------------------------
Purpose: Turn a snippet of JS into a QJSValue function.
         Wrapping it in an anonymous function allows any JS that
         evaluates to a function to be used in MIDI mapping XML files
         and ensures the function is executed with the correct
         'this' object.
Input:   QString snippet of JS that evaluates to a function,
         int number of arguments that the function takes
Output:  QJSValue of JS snippet wrapped in an anonymous function
------------------------------------------------------------------- */
QJSValue ControllerEngine::wrapFunctionCode(QString codeSnippet,int numberOfArgs)
{
    QJSValue wrappedFunction;

    auto i = m_scriptWrappedFunctionCache.constFind(codeSnippet);
    if (i != m_scriptWrappedFunctionCache.cend()) {
        wrappedFunction = i.value();
    } else {
        auto wrapperArgList = QStringList{};
        for (int i = 1; i <= numberOfArgs; i++) {
            wrapperArgList << QString("arg%1").arg(i);
        }
        auto wrapperArgs = wrapperArgList.join(",");
        auto wrappedCode = "(function (" + wrapperArgs + ") { (" +codeSnippet + ")(" + wrapperArgs + "); })";
        wrappedFunction = m_pEngine->evaluate(wrappedCode);
        checkException(wrappedFunction);
        m_scriptWrappedFunctionCache[codeSnippet] = wrappedFunction;
    }
    return wrappedFunction;
}

/* -------- ------------------------------------------------------
Purpose: Shuts down scripts in an orderly fashion
            (stops timers then executes shutdown functions)
Input:   -
Output:  -
-------- ------------------------------------------------------ */
void ControllerEngine::gracefulShutdown()
{
    qDebug() << "ControllerEngine shutting down...";

    // Clear the m_connectedControls hash so we stop responding
    // to signals.
    m_connectedControls.clear();
    // Stop all timers
    stopAllTimers();
    // Call each script's shutdown function if it exists
    if(isReady()){
        for(auto && object : m_scriptObjects) {
            auto shutdown = object.property("shutdown");
            if(shutdown.isCallable()) {
                shutdown.callWithInstance(object,{});
            }
        }
    }
    m_scriptObjects.clear();
    m_scriptModules.clear();
    m_receiveCallbacks.clear();
    // Prevents leaving decks in an unstable state
    //  if the controller is shut down while scratching
    for(auto && timer : m_scratchTimers) {
        qDebug() << "Aborting scratching on deck" << timer;
        // Clear scratch2_enable. PlayerManager::groupForDeck is 0-indexed.
        auto group = PlayerManager::groupForDeck(timer - 1);
        if(auto  pScratch2Enable = getControlObjectScript(group, "scratch2_enable"))
            pScratch2Enable->set(0);
    }
    m_scratchTimers.clear();
    // Clear the cache of function wrappers
    m_scriptWrappedFunctionCache.clear();
    // Free all the control object threads
    {
        auto values = m_controlCache.values();
        m_controlCache.clear();
        for(auto value: values) {
            delete value;
        }
    }
    m_globalObject = QJSValue{};
    if(isReady())
        m_pEngine->collectGarbage();
}
bool ControllerEngine::isReady()
{
    return !!m_pEngine;
}
void ControllerEngine::initializeScriptEngine()
{
    // Create the Script Engine
    m_pEngine = new QQmlEngine(this);
    // Make this ControllerEngine instance available to scripts as 'engine'.
    m_globalObject = m_pEngine->globalObject();
    m_globalObject.setProperty("engine", newQObject(this));

    if (m_pController) {
        qDebug() << "Controller in script engine is:" << m_pController->getDeviceName();
        // Make the Controller instance available to scripts
        m_globalObject.setProperty("controller", newQObject(m_pController));
        // ...under the legacy name as well
        m_globalObject.setProperty("midi", newQObject(m_pController));
    }
}
/* -------- ------------------------------------------------------
   Purpose: Load all script files given in the supplied list
   Input:   List of script paths and file names to load
   Output:  Returns true if no errors occurred.
   -------- ------------------------------------------------------ */
/*bool ControllerEngine::loadScriptFiles(QStringList scriptPaths,
                                       const QList<ControllerPreset::ScriptFileInfo>& scripts)
{
    if(!isReady())
        return false;
    for(auto && path : scriptPaths)
        m_pEngine->addImportPath(path);

    qDebug() << m_pEngine->importPathList();
    m_lastScriptPaths = scriptPaths;
    m_scriptModules.clear();
    m_scriptObjects.clear();
    m_globalObject = m_pEngine->globalObject();
    // scriptPaths holds the paths to search in when we're looking for scripts
    auto result = true;
    for (auto && script : scripts) {
        auto module = evaluate(script.name, scriptPaths);
        if(module.isError()) {
            if (m_scriptErrors.contains(script.name))
                qWarning() << "Errors occurred while loading" << script.name;
            result = false;
            continue;
        }
        if(script.functionPrefix.isEmpty())
            continue;
        if(module.isCallable()) {
            m_scriptModules.insert(script.functionPrefix, module);
        }else if(module.isObject()) {
            m_scriptObjects.append(module);
            m_globalObject.setProperty(script.functionPrefix, module);
        }
    }
    connect(&m_scriptWatcher, SIGNAL(fileChanged(QString)),
            this, SLOT(scriptHasChanged(QString)));

    emit(initialized());
    return result && m_scriptErrors.isEmpty();
}*/
// Slot to run when a script file has changed
void ControllerEngine::scriptHasChanged(QString scriptFilename)
{
    Q_UNUSED(scriptFilename);
    qDebug() << "ControllerEngine: Reloading Scripts";
    auto pPreset = m_pController->getPreset();

    disconnect(&m_scriptWatcher, SIGNAL(fileChanged(QString)),
               this, SLOT(scriptHasChanged(QString)));

    gracefulShutdown();
    // Delete the script engine, first clearing the pointer so that
    // other threads will not get the dead pointer after we delete it.
    if (m_pEngine ) {
        auto engine = m_pEngine;
        m_pEngine = nullptr;
        engine->deleteLater();
    }
    initializeScriptEngine();
    loadScriptFiles(m_lastScriptPaths, pPreset->scripts);

    qDebug() << "Re-initializing scripts";
    initializeScripts();
}
/* -------- ------------------------------------------------------
   Purpose: Run the initialization function for each loaded script
                if it exists
   Input:   -
   Output:  -
   -------- ------------------------------------------------------ */
void ControllerEngine::initializeScripts()
{
    auto args = (QJSValueList{}
                << m_pController->getDeviceName()
                << ControllerDebug::enabled());
    for (auto it = m_scriptModules.cbegin();
              it != m_scriptModules.cend();
              ++it) {
        auto mod = it.value();
        if(mod.isCallable()) {
            auto obj = mod.callAsConstructor(args);
            if(!obj.isError() && obj.isObject()) {
                m_globalObject.setProperty(it.key(), obj);
                m_scriptObjects.append(mod);
            }
        }
    }
    for(auto it = m_scriptObjects.begin();
                it!= m_scriptObjects.end();
                ){
        auto obj = *it;
        auto init = obj.property("init");
        if(init.isCallable()) {
            auto res = init.callWithInstance(obj,args);
            if(res.isError()) {
                it = m_scriptObjects.erase(it);
                continue;
            }
        }
        auto incomingData = obj.property("incomingData");
        if(incomingData.isCallable()) {
            m_receiveCallbacks.append(std::make_pair(obj, incomingData));
        }
        ++it;
    }
    emit(initialized());
}

/* -------- ------------------------------------------------------
   Purpose: Validate script syntax, then evaluate() it so the
            functions are registered & available for use.
   Input:   -
   Output:  -
   -------- ------------------------------------------------------ */
QJSValue ControllerEngine::evaluate(QString filepath)
{
    return evaluate(filepath, QStringList{});
}
/* -------- ------------------------------------------------------
Purpose: Evaluate & run script code
Input:   'this' object if applicable, Code string
Output:  false if an exception
-------- ------------------------------------------------------ */
bool ControllerEngine::internalExecute(QJSValue thisObject,QString scriptCode)
{
    // A special version of safeExecute since we're evaluating strings, not actual functions
    //  (execute() would print an error that it's not a function every time a timer fires.)
    if(!isReady())
        return false;

    auto scriptFunction = m_pEngine->evaluate(scriptCode);

    if (checkException(scriptFunction)) {
        qDebug() << "Exception evaluating:" << scriptCode;
        return false;
    }
    if (!scriptFunction.isCallable()) {
        // scriptCode was plain code called in evaluate above
        return false;
    }
    return internalExecute(thisObject, scriptFunction, QJSValueList());
}

/* -------- ------------------------------------------------------
Purpose: Evaluate & run script code
Input:   'this' object if applicable, Code string
Output:  false if an exception
-------- ------------------------------------------------------ */
bool ControllerEngine::internalExecute(QJSValue thisObject, QJSValue functionObject,
                                       QJSValueList args)
{
    if (!isReady()){
        qDebug() << "ControllerEngine::execute: No script engine exists!";
        return false;
    }
    if (functionObject.isError()) {
        qDebug() << "ControllerEngine::internalExecute:" << functionObject.toString();
        return false;
    }
    // If it's not a function, we're done.
    if (!functionObject.isCallable()) {
        qDebug() << "ControllerEngine::internalExecute:" << functionObject.toVariant() << "Not a function";
        return false;
    }
    // If it does happen to be a function, call it.
    auto rc = functionObject.callWithInstance(thisObject, args);
    return !checkException(rc);
}
QJSValue ControllerEngine::newArray(uint32_t length)
{
    if(!isReady())
        return QJSValue{};
    return m_pEngine->newArray(length);
}
QJSValue ControllerEngine::newQObject(QObject *p)
{
    if(!isReady())
        return {};
    QQmlEngine::setObjectOwnership(p,QQmlEngine::CppOwnership);
    return m_pEngine->newQObject(p);

}
QJSValue ControllerEngine::newObject()
{
    if(!isReady())
        return QJSValue{};
    return m_pEngine->newObject();
}
void ControllerEngine::receive(QJSValueList data, mixxx::Duration timestamp)
{
    if(!isReady())
        return;
    data.prepend(m_pEngine->toScriptValue(timestamp.toDoubleSeconds()));
    for(auto && obj : m_scriptObjects) {
        if(obj.isObject()){
            auto cb = obj.property("incomingData");
            if(cb.isCallable()){
                cb.callWithInstance(obj, data);
            }
        }
    }
}
bool ControllerEngine::execute(QJSValue functionObject,
                               unsigned char channel,
                               unsigned char control,
                               unsigned char value,
                               unsigned char status,
                               QString group,
                               mixxx::Duration timestamp)
{
    Q_UNUSED(timestamp);
    if(!isReady())
        return false;
    auto args = (QJSValueList{} << channel << control << value << status << group);
    return internalExecute(m_pEngine->globalObject(), functionObject, args);
}

bool ControllerEngine::execute(QJSValue function, QByteArray data,mixxx::Duration timestamp)
{
    Q_UNUSED(timestamp);
    if(!isReady())
        return false;
    auto array = QJSValueList{};
    for(auto && c : data) { array << int(c); }
    auto args = QJSValueList{m_pEngine->toScriptValue(array), m_pEngine->toScriptValue(array.size())};
    return internalExecute(m_pEngine->globalObject(), function, args);
}

/* -------- ------------------------------------------------------
   Purpose: Check to see if a script threw an exception
   Input:   QJSValue returned from call(scriptFunctionName)
   Output:  true if there was an exception
   -------- ------------------------------------------------------ */
bool ControllerEngine::checkException(QJSValue result)
{
    if(!isReady())
        return false;

    if (result.isError()) {
        auto errorMessage = result.property("message").toString();
        auto line = QString::number(result.property("lineNumber").toInt());
        auto backtrace = result.property("stack").toString();
        auto filename = result.property("fileName").toString();

        auto error (QStringList{}<< (filename.isEmpty() ? "" : filename) << errorMessage << line);
        m_scriptErrors.insert((filename.isEmpty() ? "passed code" : filename), error);

        auto errorText = tr("Uncaught exception at line %1 in file %2: %3")
                .arg(line, (filename.isEmpty() ? "" : filename), errorMessage);

        if (filename.isEmpty())
            errorText = tr("Uncaught exception at line %1 in passed code: %2")
                    .arg(line, errorMessage);

        scriptErrorDialog(QString("%1\nBacktrace:\n%2").arg(errorText, backtrace));

        return true;
    }
    return false;
}

/*  -------- ------------------------------------------------------
    Purpose: Common error dialog creation code for run-time exceptions
                Allows users to ignore the error or reload the mappings
    Input:   Detailed error string
    Output:  -
    -------- ------------------------------------------------------ */
void ControllerEngine::scriptErrorDialog(QString detailedError)
{
    qWarning() << "ControllerEngine:" << detailedError;
    auto  props = ErrorDialogHandler::instance()->newDialogProperties();
    props->setType(DLG_WARNING);
    props->setTitle(tr("Controller script error"));
    props->setText(tr("A control you just used is not working properly."));
    props->setInfoText("<html>"+tr("The script code needs to be fixed.")+
        "<p>"+tr("For now, you can: Ignore this error for this session but you may experience erratic behavior.")+
        "<br>"+tr("Try to recover by resetting your controller.")+"</p>"+"</html>");
    props->setDetails(detailedError);
    props->setKey(detailedError);   // To prevent multiple windows for the same error

    // Allow user to suppress further notifications about this particular error
    props->addButton(QMessageBox::Ignore);

    props->addButton(QMessageBox::Retry);
    props->addButton(QMessageBox::Close);
    props->setDefaultButton(QMessageBox::Close);
    props->setEscapeButton(QMessageBox::Close);
    props->setModal(false);

    if (ErrorDialogHandler::instance()->requestErrorDialog(props)) {
        // Enable custom handling of the dialog buttons
        connect(ErrorDialogHandler::instance(), SIGNAL(stdButtonClicked(QString, QMessageBox::StandardButton)),
                this, SLOT(errorDialogButton(QString, QMessageBox::StandardButton)));
    }
}

/* -------- ------------------------------------------------------
    Purpose: Slot to handle custom button clicks in error dialogs
    Input:   Key of dialog, StandardButton that was clicked
    Output:  -
    -------- ------------------------------------------------------ */
void ControllerEngine::errorDialogButton(QString key, QMessageBox::StandardButton button)
{
    Q_UNUSED(key);
    // Something was clicked, so disable this signal now
    disconnect(ErrorDialogHandler::instance(),
               SIGNAL(stdButtonClicked(QString, QMessageBox::StandardButton)),
               this,
               SLOT(errorDialogButton(QString, QMessageBox::StandardButton)));

    if (button == QMessageBox::Retry)
        emit(resetController());
}
QJSValue ControllerEngine::getControl(QString group, QString item)
{
    if(!isReady())
        return QJSValue{};
    auto coScript = new ControlObjectScript(ConfigKey(group,item));
    QQmlEngine::setObjectOwnership(coScript,QQmlEngine::JavaScriptOwnership);
    return m_pEngine->newQObject(coScript);
}
ControlObjectScript* ControllerEngine::getControlObjectScript(QString group, QString name)
{
    auto key = ConfigKey(group, name);
    {
        auto it = m_controlCache.constFind(key);
        if(it != m_controlCache.constEnd()) {
            return it.value();
        }
    }
    // create COT
    auto coScript = new ControlObjectScript(key, this);
    if (coScript->valid()) {
        m_controlCache.insert(key, coScript);
    } else {
        delete coScript;
        coScript = nullptr;
        m_controlCache.insert(key, coScript);
    }
    return coScript;
}

/* -------- ------------------------------------------------------
   Purpose: Returns the current value of a Mixxx control (for scripts)
   Input:   Control group (e.g. [Channel1]), Key name (e.g. [filterHigh])
   Output:  The value
   -------- ------------------------------------------------------ */
double ControllerEngine::getValue(QString group, QString name)
{
    if(auto coScript = getControlObjectScript(group, name)){
        return coScript->get();
    }else{
        qWarning() << "ControllerEngine: Unknown control" << group << name << ", returning 0.0";
        return 0.0;
    }
}

/* -------- ------------------------------------------------------
   Purpose: Sets new value of a Mixxx control (for scripts)
   Input:   Control group, Key name, new value
   Output:  -
   -------- ------------------------------------------------------ */
void ControllerEngine::setValue(QString group, QString name, double newValue)
{
    if (isnan(newValue)) {
        qWarning() << "ControllerEngine: script setting [" << group << "," << name
                 << "] to NotANumber, ignoring.";
        return;
    }
    auto  coScript = getControlObjectScript(group, name);
    if (coScript) {
        auto pControl = ControlObject::getControl(coScript->getKey());
        if (pControl && !m_st.ignore(pControl, coScript->getParameterForValue(newValue))) {
            coScript->set(newValue);
        }
    }
}

/* -------- ------------------------------------------------------
   Purpose: Returns the normalized value of a Mixxx control (for scripts)
   Input:   Control group (e.g. [Channel1]), Key name (e.g. [filterHigh])
   Output:  The value
   -------- ------------------------------------------------------ */
double ControllerEngine::getParameter(QString group, QString name)
{
    if(auto  coScript = getControlObjectScript(group, name)){
        return coScript->getParameter();
    }else{
        qWarning() << "ControllerEngine: Unknown control" << group << name << ", returning 0.0";
        return 0.0;
    }
}
/* -------- ------------------------------------------------------
   Purpose: Sets new normalized parameter of a Mixxx control (for scripts)
   Input:   Control group, Key name, new value
   Output:  -
   -------- ------------------------------------------------------ */
void ControllerEngine::setParameter(QString group, QString name, double newParameter)
{
    if (isnan(newParameter)) {
        qWarning() << "ControllerEngine: script setting [" << group << "," << name
                 << "] to NotANumber, ignoring.";
        return;
    }
    if(auto  coScript = getControlObjectScript(group, name)){
    // TODO(XXX): support soft takeover.
        coScript->setParameter(newParameter);
    }
}
/* -------- ------------------------------------------------------
   Purpose: normalize a value of a Mixxx control (for scripts)
   Input:   Control group, Key name, new value
   Output:  -
   -------- ------------------------------------------------------ */
double ControllerEngine::getParameterForValue(QString group, QString name, double value)
{
    if (isnan(value)) {
        qWarning() << "ControllerEngine: script setting [" << group << "," << name
                 << "] to NotANumber, ignoring.";
        return 0.0;
    }
    if(auto  coScript = getControlObjectScript(group, name)){
        return coScript->getParameterForValue(value);
    }else{
        qWarning() << "ControllerEngine: Unknown control" << group << name << ", returning 0.0";
        return 0.0;
    }

}

/* -------- ------------------------------------------------------
   Purpose: Resets the value of a Mixxx control (for scripts)
   Input:   Control group, Key name, new value
   Output:  -
   -------- ------------------------------------------------------ */
void ControllerEngine::reset(QString group, QString name)
{
    if(auto coScript = getControlObjectScript(group, name))
        coScript->reset();
}

/* -------- ------------------------------------------------------
   Purpose: default value of a Mixxx control (for scripts)
   Input:   Control group, Key name, new value
   Output:  -
   -------- ------------------------------------------------------ */
double ControllerEngine::getDefaultValue(QString group, QString name)
{
    if(auto coScript = getControlObjectScript(group, name))
        return coScript->getDefault();
    else {
        qWarning() << "ControllerEngine: Unknown control" << group << name << ", returning 0.0";
        return 0.0;
    }
}
/* -------- ------------------------------------------------------
   Purpose: default parameter of a Mixxx control (for scripts)
   Input:   Control group, Key name, new value
   Output:  -
   -------- ------------------------------------------------------ */
double ControllerEngine::getDefaultParameter(QString group, QString name)
{
    if(auto coScript = getControlObjectScript(group, name)){
        return coScript->getParameterForValue(coScript->getDefault());
    }else{
        qWarning() << "ControllerEngine: Unknown control" << group << name << ", returning 0.0";
        return 0.0;
    }
}

/* -------- ------------------------------------------------------
   Purpose: qDebugs script output so it ends up in mixxx.log
   Input:   String to log
   Output:  -
   -------- ------------------------------------------------------ */
void ControllerEngine::log(QString message)
{
    qDebug() << message;
}
/* -------- ------------------------------------------------------
   Purpose: Emits valueChanged() so device outputs update
   Input:   -
   Output:  -
   -------- ------------------------------------------------------ */
void ControllerEngine::trigger(QString group, QString name)
{
    auto  coScript = getControlObjectScript(group, name);
    if (coScript != nullptr) {
        coScript->valueChanged(coScript->get());
    }
}
QJSValue ControllerEngine::connectControl(QString group, QString name, QJSValue callback, bool disconnect)
{
    if(!isReady())
        return {};
    return connectControl(group,name,callback,QJSValue{}, disconnect);
}

// Purpose: (Dis)connects a ControlObject valueChanged() signal to/from a
//          script function
// Input:   Control group (e.g. [Channel1]), Key name (e.g. [filterHigh]),
//          script function name, true if you want to disconnect
// Output:  true if successful
QJSValue ControllerEngine::connectControl(QString group, QString name, QJSValue callback,QJSValue ctxt, bool disconnect)
{
    ConfigKey key(group, name);
    if(!isReady() || (!disconnect && !callback.isCallable()))
        return QJSValue{};

    if(disconnect) {
        auto success = false;
        for(auto it = m_connectedControls.constFind(key);
                 it != m_connectedControls.constEnd() && it.key() == key;
                 ) {
            if(auto co = it.value()) {
                if((!callback.isCallable() || co->callback().equals(callback))
                && (!ctxt.isObject() || co->context().equals(ctxt))) {
                    it = m_connectedControls.erase(it);
                    co->deleteLater();
                    success = true;
                    continue;
                }
            }
            ++it;
        }
        return QJSValue{success};
    }

    qDebug() << "Connection:" << group << name;
    auto coScript = new ControlObjectScript(key, this);
    if(!coScript->valid()) {
        delete coScript;
        return {false};
    }
    coScript->connectControl(callback,ctxt);
    m_connectedControls.insert(key, coScript);
    return m_pEngine->newQObject(coScript);
}


/* -------- ------------------------------------------------------
   Purpose: Evaluate a script file
   Input:   Script filename
   Output:  false if the script file has errors or doesn't exist
   -------- ------------------------------------------------------ */
QJSValue ControllerEngine::evaluate(QString scriptName, QStringList scriptPaths)
{
    if(!isReady())
        return false;
    auto filename = QString{""};
    QFile input;
    if (scriptPaths.length() == 0) {
        // If we aren't given any paths to search, assume that scriptName
        // contains the full file name
        filename = scriptName;
        input.setFileName(filename);
    } else {
        for (auto && scriptPath : scriptPaths) {
            QDir scriptPathDir(scriptPath);
            filename = scriptPathDir.absoluteFilePath(scriptName);
            input.setFileName(filename);
            if (input.exists())  {
                qDebug() << "ControllerEngine: Watching JS File:" << filename;
                m_scriptWatcher.addPath(filename);
                break;
            }
        }
    }
    qDebug() << "ControllerEngine: Loading" << filename;
    // Read in the script file
    if (!input.open(QIODevice::ReadOnly)) {
        qWarning() << QString("ControllerEngine: Problem opening the script file: %1, error # %2, %3")
                .arg(filename, QString::number(input.error()), input.errorString());
        if (m_bPopups) {
            // Set up error dialog
            auto props = ErrorDialogHandler::instance()->newDialogProperties();
            props->setType(DLG_WARNING);
            props->setTitle("Controller script file problem");
            props->setText(QString("There was a problem opening the controller script file %1.").arg(filename));
            props->setInfoText(input.errorString());

            // Ask above layer to display the dialog & handle user response
            ErrorDialogHandler::instance()->requestErrorDialog(props);
        }
        return false;
    }

    auto scriptCode = QString{""};
    scriptCode.append(input.readAll());
    scriptCode.append('\n');
    input.close();

    // Check syntax
    auto result = m_pEngine->evaluate(scriptCode);
    if(result.isError()) {
        auto error = QString("Uncaught exception at %1 at line %2,\n\n%3").arg(
                result.property("name").toString()
              , QString::number(result.property("lineNumber").toInt())
              , result.toString()
                );

        qWarning() << "ControllerEngine:" << error;
        if (m_bPopups) {
            auto props = ErrorDialogHandler::instance()->newDialogProperties();
            props->setType(DLG_WARNING);
            props->setTitle("Controller script file error");
            props->setText(QString("There was an error in the controller script file %1.").arg(filename));
            props->setInfoText("The functionality provided by this script file will be disabled.");
            props->setDetails(error);

            ErrorDialogHandler::instance()->requestErrorDialog(props);
        }
        return false;
    }else{
    // Evaluate the code
        return  result;
    }
}
bool ControllerEngine::hasErrors(QString filename)
{
    return m_scriptErrors.contains(filename);
}
const QStringList ControllerEngine::getErrors(QString filename)
{
    return  m_scriptErrors.value(filename, QStringList());
}
/* -------- ------------------------------------------------------
   Purpose: Creates & starts a timer that runs some script code
                on timeout
   Input:   Number of milliseconds, script function to call,
                whether it should fire just once
   Output:  The timer's ID, 0 if starting it failed
   -------- ------------------------------------------------------ */
int ControllerEngine::beginTimer(int interval, QJSValue timerCallback, bool oneShot)
{
    return beginTimer(interval,timerCallback,QJSValue{}, oneShot);
}
int ControllerEngine::beginTimer(int interval, QJSValue timerCallback,QJSValue ctxt, bool oneShot)
{
    if (!timerCallback.isCallable() && !timerCallback.isString()) {
        qWarning() << "Invalid timer callback provided to beginTimer."
                   << "Valid callbacks are strings and functions.";
        return 0;
    }
    if (interval < 20) {
        qWarning() << "Timer request for" << interval
                   << "ms is too short. Setting to the minimum of 20ms.";
        interval = 20;
    }
    // This makes use of every QObject's internal timer mechanism. Nice, clean,
    // and simple. See http://doc.trolltech.com/4.6/qobject.html#startTimer for
    // details
    auto timerId = startTimer(interval);
    TimerInfo info;
    info.callback = timerCallback;
    info.context = ctxt;
    info.oneShot = oneShot;
    m_timers[timerId] = info;
    if (timerId == 0) {
        qWarning() << "Script timer could not be created";
    } else if (oneShot) {
        controllerDebug("Starting one-shot timer:" << timerId);
    } else {
        controllerDebug("Starting timer:" << timerId);
    }
    return timerId;
}

/* -------- ------------------------------------------------------
   Purpose: Stops & removes a timer
   Input:   ID of timer to stop
   Output:  -
   -------- ------------------------------------------------------ */
void ControllerEngine::stopTimer(int timerId)
{
    if (!m_timers.contains(timerId)) {
        qWarning() << "Killing timer" << timerId << ": That timer does not exist!";
        return;
    }
    controllerDebug("Killing timer:" << timerId);
    killTimer(timerId);
    m_timers.remove(timerId);
}

void ControllerEngine::stopAllTimers()
{
    for(auto && key : m_timers.keys())
        stopTimer(key);
}
void ControllerEngine::timerEvent(QTimerEvent *event)
{
    auto timerId = event->timerId();

    // See if this is a scratching timer
    if (m_scratchTimers.contains(timerId)) {
        scratchProcess(timerId);
        return;
    }
    auto it = m_timers.find(timerId);
    if (it == m_timers.end()) {
        qWarning() << "Timer" << timerId << "fired but there's no function mapped to it!";
        return;
    }
    // NOTE(rryan): Do not assign by reference -- make a copy. I have no idea
    // why but this causes segfaults in ~QJSValue while scratching if we
    // don't copy here -- even though internalExecute passes the QJSValues
    // by value. *boggle*
    auto timerTarget = it.value();
    if (timerTarget.oneShot) {
        stopTimer(timerId);
    }
    if (timerTarget.callback.isString()) {
        internalExecute(timerTarget.context, timerTarget.callback.toString());
    } else if (timerTarget.callback.isCallable()) {
        internalExecute(timerTarget.context, timerTarget.callback,QJSValueList());
    }
}

double ControllerEngine::getDeckRate(QString group)
{
    auto rate = 0.0;
    if(auto  pRate = getControlObjectScript(group, "rate"))
        rate = pRate->get();

    if(auto  pRateDir = getControlObjectScript(group, "rate_dir"))
        rate *= pRateDir->get();
    if(auto pRateRange = getControlObjectScript(group, "rateRange"))
        rate *= pRateRange->get();

    // Add 1 since the deck is playing
    rate += 1.0;

    // See if we're in reverse play
    if(auto pReverse = getControlObjectScript(group, "reverse")) {
        if(pReverse->get() == 1)
            rate = -rate;
    }
    return rate;
}
bool ControllerEngine::isDeckPlaying(QString group)
{
    if(auto pPlay = getControlObjectScript(group, "play")){
        return pPlay->get() > 0.0;
    }else{
      auto error = QString("Could not getControlObjectScript()");
      scriptErrorDialog(error);
      return false;
    }
}

/* -------- ------------------------------------------------------
    Purpose: Enables scratching for relative controls
    Input:   Virtual deck to scratch,
             Number of intervals per revolution of the controller wheel,
             RPM for the track at normal speed (usually 33+1/3),
             (optional) alpha value for the filter,
             (optional) beta value for the filter
    Output:  -
    -------- ------------------------------------------------------ */
void ControllerEngine::scratchEnable(int deck, int intervalsPerRev, double rpm,
                                     double alpha, double beta, bool ramp)
{

    // If we're already scratching this deck, override that with this request
    if (m_dx[deck]) {
        //qDebug() << "Already scratching deck" << deck << ". Overriding.";
        auto timerId = m_scratchTimers.key(deck);
        killTimer(timerId);
        m_scratchTimers.remove(timerId);
    }
    // Controller resolution in intervals per second at normal speed.
    // (rev/min * ints/rev * mins/sec)
    auto intervalsPerSecond = (rpm * intervalsPerRev) / 60.0;
    if (intervalsPerSecond == 0.0) {
        qWarning() << "Invalid rpm or intervalsPerRev supplied to scratchEnable. Ignoring request.";
        return;
    }
    m_dx[deck]                  = 1.0 / intervalsPerSecond;
    m_intervalAccumulator[deck] = 0.0;
    m_ramp[deck]                = false;
    m_rampFactor[deck]          = 0.001;
    m_brakeActive[deck]         = false;

    // PlayerManager::groupForDeck is 0-indexed.
    auto group = PlayerManager::groupForDeck(deck - 1);
    // Ramp velocity, default to stopped.
    auto initVelocity = 0.0;
    auto  pScratch2Enable = getControlObjectScript(group, "scratch2_enable");

    // If ramping is desired, figure out the deck's current speed
    if (ramp) {
        // See if the deck is already being scratched
        if (pScratch2Enable && pScratch2Enable->get() == 1) {
            // If so, set the filter's initial velocity to the scratch speed
            if(auto  pScratch2 = getControlObjectScript(group, "scratch2")) {
                initVelocity = pScratch2->get();
            }
        } else if (isDeckPlaying(group)) {
            // If the deck is playing, set the filter's initial velocity to the
            // playback speed
            initVelocity = getDeckRate(group);
        }
    }
    // Initialize scratch filter
    if (alpha && beta) {
        m_scratchFilters[deck]->init(kAlphaBetaDt, initVelocity, alpha, beta);
    } else {
        // Use filter's defaults if not specified
        m_scratchFilters[deck]->init(kAlphaBetaDt, initVelocity);
    }
    // 1ms is shortest possible, OS dependent
    auto timerId = startTimer(kScratchTimerMs);
    // Associate this virtual deck with this timer for later processing
    m_scratchTimers[timerId] = deck;
    // Set scratch2_enable
    if (pScratch2Enable) {
        pScratch2Enable->set(1);
    }
}

/* -------- ------------------------------------------------------
    Purpose: Accumulates "ticks" of the controller wheel
    Input:   Virtual deck to scratch, interval value (usually +1 or -1)
    Output:  -
    -------- ------------------------------------------------------ */
void ControllerEngine::scratchTick(int deck, int interval)
{
    m_lastMovement[deck] = mixxx::Time::elapsed();
    m_intervalAccumulator[deck] += interval;
}

/* -------- ------------------------------------------------------
    Purpose: Applies the accumulated movement to the track speed
    Input:   ID of timer for this deck
    Output:  -
    -------- ------------------------------------------------------ */
void ControllerEngine::scratchProcess(int timerId)
{
    auto deck = m_scratchTimers[timerId];
    // PlayerManager::groupForDeck is 0-indexed.
    auto group = PlayerManager::groupForDeck(deck - 1);
    if(auto filter = m_scratchFilters[deck]){
        auto oldRate = filter->predictedVelocity();
        // Give the filter a data point:
        // If we're ramping to end scratching and the wheel hasn't been turned very
        // recently (spinback after lift-off,) feed fixed data
        if (m_ramp[deck] &&
            ((mixxx::Time::elapsed() - m_lastMovement[deck]) >= mixxx::Duration::fromMillis(1))) {
            filter->observe(m_rampTo[deck] * m_rampFactor[deck]);
            // Once this code path is run, latch so it always runs until reset
            //m_lastMovement[deck] += mixxx::Duration::fromSeconds(1);
        } else {
            // This will (and should) be 0 if no net ticks have been accumulated
            // (i.e. the wheel is stopped)
            filter->observe(m_dx[deck] * m_intervalAccumulator[deck]);
        }
        auto newRate = filter->predictedVelocity();
        // Actually do the scratching
        if(auto pScratch2 = getControlObjectScript(group, "scratch2")){
            pScratch2->set(newRate);
            // Reset accumulator
            m_intervalAccumulator[deck] = 0;
            // If we're ramping and the current rate is really close to the rampTo value
            // or we're in brake mode and have crossed over the zero value, end
            // scratching
            if ((m_ramp[deck] && std::abs(m_rampTo[deck] - newRate) <= 0.00001) ||
                (m_brakeActive[deck] && (
                    (oldRate > 0.0 && newRate < 0.0) ||
                    (oldRate < 0.0 && newRate > 0.0)))) {
                // Not ramping no mo'
                m_ramp[deck] = false;
                if (m_brakeActive[deck]) {
                    // If in brake mode, set scratch2 rate to 0 and turn off the play button.
                    pScratch2->set(0.0);
                    if(auto pPlay = getControlObjectScript(group, "play")) {
                        pPlay->set(0.0);
                    }
                }
                // Clear scratch2_enable to end scratching.
                if(auto  pScratch2Enable = getControlObjectScript(group, "scratch2_enable")){
                    pScratch2Enable->set(0);
                    // Remove timer
                    killTimer(timerId);
                    m_scratchTimers.remove(timerId);
                    m_dx[deck] = 0.0;
                    m_brakeActive[deck] = false;
                }
            }
        }
    } else {
        qWarning() << "Scratch filter pointer is null on deck" << deck;
    }
}
/* -------- ------------------------------------------------------
    Purpose: Stops scratching the specified virtual deck
    Input:   Virtual deck to stop scratching
    Output:  -
    -------- ------------------------------------------------------ */
void ControllerEngine::scratchDisable(int deck, bool ramp)
{
    // PlayerManager::groupForDeck is 0-indexed.
    auto group = PlayerManager::groupForDeck(deck - 1);
    m_rampTo[deck] = 0.0;
    // If no ramping is desired, disable scratching immediately
    if (!ramp) {
        // Clear scratch2_enable
        if(auto pScratch2Enable = getControlObjectScript(group, "scratch2_enable")){
            pScratch2Enable->set(0);
        }
        // Can't return here because we need scratchProcess to stop the timer.
        // So it's still actually ramping, we just won't hear or see it.
    } else if (isDeckPlaying(group)) {
        // If so, set the target velocity to the playback speed
        m_rampTo[deck] = getDeckRate(group);
    }

    m_lastMovement[deck] = mixxx::Time::elapsed();
    m_ramp[deck] = true;    // Activate the ramping in scratchProcess()
}
/* -------- ------------------------------------------------------
    Purpose: Tells if the specified deck is currently scratching
             (Scripts need this to implement spinback-after-lift-off)
    Input:   Virtual deck to inquire about
    Output:  True if so
    -------- ------------------------------------------------------ */
bool ControllerEngine::isScratching(int deck)
{
    // PlayerManager::groupForDeck is 0-indexed.
    auto group = PlayerManager::groupForDeck(deck - 1);
    // Don't report that we are scratching if we're ramping.
    return getValue(group, "scratch2_enable") > 0 && !m_ramp[deck];
}

/*  -------- ------------------------------------------------------
    Purpose: [En/dis]ables soft-takeover status for a particular ControlObject
    Input:   ControlObject group and key values,
                whether to set the soft-takeover status or not
    Output:  -
    -------- ------------------------------------------------------ */
void ControllerEngine::softTakeover(QString group, QString name, bool set)
{
    auto pControl = ControlObject::getControl(ConfigKey(group, name));
    if (!pControl) {
        return;
    }
    if (set) {
        m_st.enable(pControl);
    } else {
        m_st.disable(pControl);
    }
}
/*  -------- ------------------------------------------------------
     Purpose: Ignores the next value for the given ControlObject
                This should be called before or after an absolute physical
                control (slider or knob with hard limits) is changed to operate
                on a different ControlObject, allowing it to sync up to the
                soft-takeover state without an abrupt jump.
     Input:   ControlObject group and key values
     Output:  -
     -------- ------------------------------------------------------ */
void ControllerEngine::softTakeoverIgnoreNextValue(QString group, const QString name)
{
    if(auto pControl = ControlObject::getControl(ConfigKey(group, name))){
        m_st.ignoreNext(pControl);
    }
}
/*  -------- ------------------------------------------------------
    Purpose: [En/dis]ables spinback effect for the channel
    Input:   deck, activate/deactivate, factor (optional),
             delay (optional), rate (optional)
    Output:  -
    -------- ------------------------------------------------------ */
void ControllerEngine::spinback(int deck, bool activate, double factor, double rate)
{
    // defaults for args set in header file
    brake(deck, activate, factor, rate);
}
/*  -------- ------------------------------------------------------
    Purpose: [En/dis]ables brake/spinback effect for the channel
    Input:   deck, activate/deactivate, factor (optional),
             delay (optional), rate (optional)
    Output:  -
    -------- ------------------------------------------------------ */
void ControllerEngine::brake(int deck, bool activate, double factor, double rate)
{
    // PlayerManager::groupForDeck is 0-indexed.
    auto group = PlayerManager::groupForDeck(deck - 1);
    // kill timer when both enabling or disabling
    auto timerId = m_scratchTimers.key(deck);
    killTimer(timerId);
    m_scratchTimers.remove(timerId);
    // enable/disable scratch2 mode
    if(auto pScratch2Enable = getControlObjectScript(group, "scratch2_enable")) {
        pScratch2Enable->set(activate ? 1 : 0);
    }
    // used in scratchProcess for the different timer behavior we need
    m_brakeActive[deck] = activate;
    if (activate) {
        // store the new values for this spinback/brake effect
        m_rampFactor[deck] = rate * factor / 100000.0; // approx 1 second for a factor of 1
        m_rampTo[deck] = 0.0;
        // setup timer and set scratch2
        auto timerId = startTimer(kScratchTimerMs);
        m_scratchTimers[timerId] = deck;
        if(auto pScratch2 = getControlObjectScript(group, "scratch2")){
            pScratch2->set(rate);
        }
        // setup the filter using the default values of alpha and beta
        if(auto filter = m_scratchFilters[deck]){
            filter->init(kAlphaBetaDt, rate);
        }
        // activate the ramping in scratchProcess()
        m_ramp[deck] = true;
    }
}
QJSValue ControllerEngine::findObject(QString name)
{
    if(!isReady())
        return QJSValue{};
    return m_globalObject.property(name);
}
