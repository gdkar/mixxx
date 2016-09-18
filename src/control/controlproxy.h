#ifndef CONTROLPROXY_H
#define CONTROLPROXY_H

#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "control/control.h"
#include "preferences/usersettings.h"

// This class is the successor of ControlObjectThread. It should be used for
// new code to avoid unnecessary locking during send if no slot is connected.
// Do not (re-)connect slots during runtime, since this locks the mutex in
// QMetaObject::activate().
// Be sure that the ControlProxy is created and deleted from the same
// thread, otherwise a pending signal may lead to a segfault (Bug #1406124).
// Parent it to the the creating object to achieve this.
class ControlProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(ConfigKey key READ getKey NOTIFY keyChanged)
    Q_PROPERTY(double value READ get WRITE set RESET reset NOTIFY valueChanged)
    Q_PROPERTY(double defaultValue READ getDefault NOTIFY defaultValueChanged)
  public:
    Q_INVOKABLE ControlProxy(QObject* pParent = nullptr);
    ControlProxy(QString g, QString i, QObject* pParent = nullptr);
    ControlProxy(const char* g, const char* i, QObject* pParent = nullptr);
    ControlProxy(ConfigKey key, QObject* pParent = nullptr);
    virtual ~ControlProxy();
    void initialize(ConfigKey key);
    ConfigKey getKey() const;
    bool connectValueChanged(const QObject* receiver,
            const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    bool connectValueChanged(
            const char* method, Qt::ConnectionType type = Qt::AutoConnection);


    bool valid() const;

  public slots:
    // Returns the value of the object. Thread safe, non-blocking.
    double get() const;
    // Returns the bool interpretation of the value
    bool toBool() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    double getParameter() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    double getParameterForValue(double value) const;
    double getDefault() const;
    void slotSet(double v);
    // Sets the control value to v. Thread safe, non-blocking.
    void set(double v);
    // Sets the control parameterized value to v. Thread safe, non-blocking.
    void setParameter(double v);
    // Resets the control to its default value. Thread safe, non-blocking.
    void reset();
    void trigger();
  signals:
    void keyChanged(ConfigKey);
    // This signal must not connected by connect(). Use connectValueChanged()
    // instead. It will connect to the base ControlDoublePrivate as well.
    void valueChanged(double);
    void defaultValueChanged(double);
    void triggered();
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

#endif // CONTROLPROXY_H
