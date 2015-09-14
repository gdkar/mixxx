_Pragma("once")
#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "configobject.h"

class ControlDoublePrivate;
class ControlObjectSlave : public QObject {
    Q_OBJECT
  public:
    ControlObjectSlave(QObject* pParent = nullptr);
    ControlObjectSlave(const QString& g, const QString& i, QObject* pParent = nullptr);
    ControlObjectSlave(const char* g, const char* i, QObject* pParent = nullptr);
    ControlObjectSlave(const ConfigKey& key, QObject* pParent = nullptr);
    virtual ~ControlObjectSlave() = default;
    void initialize(const ConfigKey& key);
    const ConfigKey& getKey() const;
    bool connectValueChanged(const QObject* receiver,const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    bool connectValueChanged(const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    // Called from update();
    virtual void emitValueChanged();
    virtual bool valid() const;
    // Returns the value of the object. Thread safe, non-blocking.
    virtual double get() const;
    // Returns the bool interpretation of the value
    virtual bool toBool() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    virtual double getParameter() const;
    virtual double getDefault() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    virtual double getParameterForValue(double value) const;
  public slots:
    // Sets the control value to v. Thread safe, non-blocking.
    virtual void set ( double v);
    // Sets the control parameterized value to v. Thread safe, non-blocking.
    virtual void setParameter(double v);
    // Resets the control to its default value. Thread safe, non-blocking.
    void reset();
  signals:
    // This signal must not connected by connect(). Use connectValueChanged()
    // instead. It will connect to the base ControlDoublePrivate as well.
    void valueChanged(double);
  protected slots:
    // Receives the value from the master control and re-emits either
    // valueChanged(double) or valueChangedByThis(double) based on pSetter.
    virtual void slotValueChanged(double v, QObject* pSetter);
  protected:
    ConfigKey m_key;
    // Pointer to connected control.
    QSharedPointer<ControlDoublePrivate> m_pControl;
};
