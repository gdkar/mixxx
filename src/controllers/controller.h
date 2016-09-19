/**
* @file controller.h
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Sat Apr 30 2011
* @brief Base class representing a physical (or software) controller.
*
* This is a base class representing a physical (or software) controller.  It
* must be inherited by a class that implements it on some API. Note that the
* subclass' destructor should call close() at a minimum.
*/

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "controllers/controllerengine.h"
#include "controllers/controllervisitor.h"
#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetinfo.h"
#include "controllers/controllerpresetvisitor.h"
#include "controllers/controllerpresetfilehandler.h"
#include "util/duration.h"


class BindingProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
public:
    Q_INVOKABLE BindingProxy(QObject *p = nullptr);
    Q_INVOKABLE BindingProxy(QString prefix, QObject *p);
   ~BindingProxy();
    double value() const;
    void setValue(double);
    QString prefix() const;
    void setPrefix(QString);
signals:
    void valueChanged(double);
    void messageReceived(double, double = 0);
    void prefixChanged(QVariant);

protected:
    QString m_prefix{};
    double   m_value{};
};
QML_DECLARE_TYPE(BindingProxy)
class Controller : public QObject, ConstControllerPresetVisitor {
    Q_OBJECT

    Q_PROPERTY(bool isOpen READ isOpen WRITE setOpen NOTIFY isOpenChanged);
    Q_PROPERTY(bool isOutputDevice READ isOutputDevice WRITE setOutputDevice NOTIFY isOutputDeviceChanged);
    Q_PROPERTY(bool isInputDevice READ isInputDevice WRITE setInputDevice NOTIFY isInputDeviceChanged);
    Q_PROPERTY(QString presetExtension READ presetExtension CONSTANT);
    Q_PROPERTY(QString deviceName READ getName WRITE setDeviceName NOTIFY deviceNameChanged);
    Q_PROPERTY(QString deviceCategory READ getCategory WRITE setDeviceCategory NOTIFY deviceCategoryChanged);
    Q_PROPERTY(bool mappable READ isMappable CONSTANT);
    Q_PROPERTY(bool learning READ isLearning WRITE setLearning NOTIFY learningChanged);
    Q_PROPERTY(ControllerEngine* engine READ getEngine NOTIFY engineChanged);
  public:
    Q_INVOKABLE Controller(QObject *p=nullptr);
    virtual ~Controller();  // Subclass should call close() at minimum.
    // Returns the extension for the controller (type) preset files.  This is
    // used by the ControllerManager to display only relevant preset files for
    // the controller (type.)
    virtual QString presetExtension() const ;
    void setPreset(const ControllerPreset& preset);
    virtual void accept(ControllerVisitor* visitor);
    virtual bool savePreset(const QString filename) const ;
    // Returns a clone of the Controller's loaded preset.
    virtual ControllerPresetPointer getPreset() const;
    bool isOpen() const;
    bool isOutputDevice() const;
    bool isInputDevice() const;
    QString getName() const;
    QString getCategory() const;
    virtual bool isMappable() const;
    bool isLearning() const;
    virtual bool matchPreset(const PresetInfo& preset);
  signals:
    void isOpenChanged(bool);
    void isOutputDeviceChanged(bool);
    void isInputDeviceChanged(bool);
    void deviceNameChanged(QString);
    void deviceCategoryChanged(QString);
    void learningChanged(bool);
    void engineChanged(ControllerEngine*);
    // Emitted when a new preset is loaded. pPreset is a /clone/ of the loaded
    // preset, not a pointer to the preset itself.
    void presetLoaded(ControllerPresetPointer pPreset);
  // Making these slots protected/private ensures that other parts of Mixxx can
  // only signal them which allows us to use no locks.
  protected slots:
    virtual BindingProxy *getBindingFor(QString prefix);
    // Handles packets of raw bytes and passes them to an ".incomingData" script
    // function that is assumed to exist. (Sub-classes may want to reimplement
    // this if they have an alternate way of handling such data.)
    virtual void receive(QVariant data, mixxx::Duration timestamp);
    // Initializes the controller engine and returns whether it was successful.
    virtual bool applyPreset(QList<QString> scriptPaths, bool initializeScripts);
    // Puts the controller in and out of learning mode.
    void startLearning();
    void stopLearning();
    void setLearning(bool);
  protected:
    Q_INVOKABLE void send(QList<int> data, unsigned int length);
    // To be called in sub-class' open() functions after opening the device but
    // before starting any input polling/processing.
    void startEngine();
    // To be called in sub-class' close() functions after stopping any input
    // polling/processing but before closing the device.
    void stopEngine();
    ControllerEngine* getEngine() const;
    void setDeviceName(QString deviceName);
    void setDeviceCategory(QString deviceCategory);
    void setOutputDevice(bool outputDevice);
    void setInputDevice(bool inputDevice);
    void setOpen(bool open);
  private slots:
    virtual int open();
    virtual int close();
    // Requests that the device poll if it is a polling device. Returns true
    // if events were handled.
    virtual bool poll();
  protected:
    QHash<QString, BindingProxy*> m_dispatch;
  private:
    // This must be reimplemented by sub-classes desiring to send raw bytes to a
    // controller.
    virtual void send(QByteArray data);
    // Returns true if this device should receive polling signals via calls to
    // its poll() method.
    virtual bool isPolling() const;
    // Returns a pointer to the currently loaded controller preset. For internal
    // use only.
    virtual ControllerPreset* preset();
    ControllerEngine* m_pEngine{};
    // Verbose and unique device name suitable for display.
    QString m_sDeviceName;
    // Verbose and unique description of device type, defaults to empty
    QString m_sDeviceCategory;
    // Flag indicating if this device supports output (receiving data from
    // Mixxx)
    bool m_bIsOutputDevice{false};
    // Flag indicating if this device supports input (sending data to Mixxx)
    bool m_bIsInputDevice{false};
    // Indicates whether or not the device has been opened for input/output.
    bool m_bIsOpen{false};
    bool m_bLearning{false};
    friend class ControllerManager; // accesses lots of our stuff, but in the same thread
    // For testing.
    friend class ControllerPresetValidationTest;

};
QML_DECLARE_TYPE(Controller);
#endif
