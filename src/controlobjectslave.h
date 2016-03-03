_Pragma("once")
#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "control/control.h"
#include "preferences/usersettings.h"

// This class is the successor of ControlObjectThread. It should be used for
// new code to avoid unnecessary locking during send if no slot is connected.
// Do not (re-)connect slots during runtime, since this locks the mutex in
// QMetaObject::activate().
// Be sure that the ControlObjectSlave is created and deleted from the same
// thread, otherwise a pending signal may lead to a segfault (Bug #1406124).
// Parent it to the the creating object to achieve this.
class ControlObjectSlave : public QObject {
    Q_OBJECT
  public:
    ControlObjectSlave(QObject* pParent = NULL);
    ControlObjectSlave(const QString& g, const QString& i, QObject* pParent = NULL);
    ControlObjectSlave(const char* g, const char* i, QObject* pParent = NULL);
    ControlObjectSlave(const ConfigKey& key, QObject* pParent = NULL);
    virtual ~ControlObjectSlave();
    void initialize(const ConfigKey& key);
    ConfigKey getKey() const;
    bool connectValueChanged(const QObject* receiver,
            const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    bool connectValueChanged(
            const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    // Called from update();
    virtual void emitValueChanged();
    bool valid() const;
    // Returns the value of the object. Thread safe, non-blocking.
    double get() const;
    // Returns the bool interpretation of the value
    bool toBool() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    double getParameter() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    double getParameterForValue(double value) const;
    // Returns the normalized parameter of the object. Thread safe, non-blocking.
    double getDefault() const;
  public slots:
    // Set the control to a new value. Non-blocking.
    void slotSet(double v);
    // Sets the control value to v. Thread safe, non-blocking.
    void set(double v);
    // Sets the control parameterized value to v. Thread safe, non-blocking.
    void setParameter(double v);
    // Resets the control to its default value. Thread safe, non-blocking.
    void reset();
  signals:
    // This signal must not connected by connect(). Use connectValueChanged()
    // instead. It will connect to the base ControlDoublePrivate as well.
    void valueChanged(double);
  protected slots:
    // Receives the value from the master control by a unique direct connection
    void slotValueChangedDirect(double v, QObject* pSetter);
    // Receives the value from the master control by a unique auto connection
    void slotValueChangedAuto(double v, QObject* pSetter);
    // Receives the value from the master control by a unique Queued connection
    void slotValueChangedQueued(double v, QObject* pSetter);
  protected:
    ConfigKey m_key;
    // Pointer to connected control.
    QSharedPointer<ControlDoublePrivate> m_pControl;
};
