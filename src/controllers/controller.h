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

_Pragma("once")
#include "controllers/controllerengine.h"
#include "controllers/controllervisitor.h"
#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetinfo.h"
#include "controllers/controllerpresetvisitor.h"
#include "controllers/controllerpresetfilehandler.h"

class Controller : public QObject, ConstControllerPresetVisitor {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName WRITE setDeviceName NOTIFY nameChanged);
    Q_PROPERTY(QString category READ getCategory WRITE setDeviceCategory NOTIFY categoryChanged);
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY isOpenChanged);
    Q_PROPERTY(bool input READ isInputDevice WRITE setInputDevice NOTIFY isInputChanged);
    Q_PROPERTY(bool output READ isOutputDevice WRITE setOutputDevice NOTIFY isOutputChanged);
    Q_PROPERTY(bool learning READ isLearning WRITE setLearning NOTIFY isLearningChanged);
    Q_PROPERTY(bool polling READ isPolling CONSTANT);
    Q_PROPERTY(ControllerEngine* engine READ getEngine CONSTANT);
  public:
    Controller();
    virtual ~Controller();  // Subclass should call close() at minimum.
    // Returns the extension for the controller (type) preset files.  This is
    // used by the ControllerManager to display only relevant preset files for
    // the controller (type.)
    virtual QString presetExtension() = 0;
    void setPreset(const ControllerPreset& preset);
    virtual void accept(ControllerVisitor* visitor) = 0;
    virtual bool savePreset(const QString filename) const = 0;
    // Returns a clone of the Controller's loaded preset.
    virtual ControllerPresetPointer getPreset() const = 0;
    bool isOpen() const;
    bool isOutputDevice() const;
    bool isInputDevice() const;
    QString getName() const;
    QString getCategory() const;
    bool debugging() const;
    virtual bool isMappable() const;
    bool isLearning() const;
    virtual bool matchPreset(const PresetInfo& preset);
  signals:
    // Emitted when a new preset is loaded. pPreset is a /clone/ of the loaded
    // preset, not a pointer to the preset itself.
    void presetLoaded(ControllerPresetPointer pPreset);
    void nameChanged(QString);
    void categoryChanged(QString);
    void isOutputChanged(bool);
    void isInputChanged(bool);
    void isOpenChanged(bool);
    void isLearningChanged(bool);
  // Making these slots protected/private ensures that other parts of Mixxx can
  // only signal them which allows us to use no locks.
  protected slots:
    // Handles packets of raw bytes and passes them to an ".incomingData" script
    // function that is assumed to exist. (Sub-classes may want to reimplement
    // this if they have an alternate way of handling such data.)
    virtual void receive(const QByteArray data);
    // Initializes the controller engine
    virtual void applyPreset(QList<QString> scriptPaths);
    // Puts the controller in and out of learning mode.
    virtual void setLearning(bool);
    virtual void startLearning();
    virtual void stopLearning();
    Q_INVOKABLE void send(QList<int> data, unsigned int length);
  protected:
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
    virtual int open() = 0;
    virtual int close() = 0;
    // Requests that the device poll if it is a polling device. Returns true
    // if events were handled.
    virtual bool poll();
  private:
    // This must be reimplemented by sub-classes desiring to send raw bytes to a
    // controller.
    virtual void send(QByteArray data) = 0;
    // Returns true if this device should receive polling signals via calls to
    // its poll() method.
    virtual bool isPolling() const;
    // Returns a pointer to the currently loaded controller preset. For internal
    // use only.
    virtual ControllerPreset* preset() = 0;
    ControllerEngine* m_pEngine;
    // Verbose and unique device name suitable for display.
    QString m_sDeviceName;
    // Verbose and unique description of device type, defaults to empty
    QString m_sDeviceCategory;
    // Flag indicating if this device supports output (receiving data from
    // Mixxx)
    bool m_bIsOutputDevice;
    // Flag indicating if this device supports input (sending data to Mixxx)
    bool m_bIsInputDevice;
    // Indicates whether or not the device has been opened for input/output.
    bool m_bIsOpen;
    // Specifies whether or not we should dump incoming data to the console at
    // runtime. This is useful for end-user debugging and script-writing.
    bool m_bDebug;
    bool m_bLearning;
    friend class ControllerManager; // accesses lots of our stuff, but in the same thread
};
