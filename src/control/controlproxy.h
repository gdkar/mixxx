#ifndef CONTROLPROXY_H
#define CONTROLPROXY_H

#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QtQml>
#include <limits>
#include <algorithm>
#include <utility>
#include <functional>
#include <tuple>

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
    Q_PROPERTY(ConfigKey key READ key WRITE setKey NOTIFY keyChanged )
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
    Q_PROPERTY(QString   group READ group WRITE setGroup NOTIFY groupChanged )
    Q_PROPERTY(QString   item READ item WRITE setItem NOTIFY itemChanged     )
    Q_PROPERTY(double value READ get WRITE set RESET reset NOTIFY valueChanged)
    Q_PROPERTY(double defaultValue READ getDefault NOTIFY defaultValueChanged)
  public:
    Q_INVOKABLE ControlProxy(QObject* pParent = nullptr);
    Q_INVOKABLE ControlProxy(QString g, QString i, QObject* pParent = nullptr);
    Q_INVOKABLE ControlProxy(ConfigKey key, QObject* pParent = nullptr);
    virtual ~ControlProxy();
    void initialize(ConfigKey key);
    void initialize() const;
    bool connectValueChanged(const QObject* receiver,
            const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    bool connectValueChanged(
            const char* method, Qt::ConnectionType type = Qt::AutoConnection);


    virtual bool valid() const;

    // Returns the bool interpretation of the value
    bool toBool() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    double getParameter() const;
    void setParameter(double v);
    double getParameterForValue(double value) const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    double getDefault() const;

    virtual ConfigKey key() const;
    virtual void      setKey(ConfigKey key);
    ConfigKey getKey() const;

    virtual QString group() const;
    virtual void    setGroup(QString);
    virtual QString item()  const;
    virtual void    setItem(QString);

    virtual double operator += (double incr);
    virtual double operator -= (double incr);
    virtual double operator ++ ();
    virtual double operator -- ();
    virtual double operator ++ (int);
    virtual double operator -- (int);
    Q_INVOKABLE virtual double add_and_saturate(double val, double low_bar = -std::numeric_limits<double>::infinity(), double high_bar = std::numeric_limits<double>::infinity());
    Q_INVOKABLE virtual double fetch_sub(double val);
    Q_INVOKABLE virtual double fetch_add(double val);
    Q_INVOKABLE virtual double exchange (double with);
    Q_INVOKABLE virtual double compare_exchange(double expected, double desired);
    Q_INVOKABLE virtual double fetch_mul(double by);
    Q_INVOKABLE virtual double fetch_div(double by);
    Q_INVOKABLE virtual double fetch_toggle();
    // Sets the control value to v. Thread safe, non-blocking.
    //    // Returns the value of the object. Thread safe, non-blocking.
  public slots:
    double get() const;
    void   set(double val);
    void   reset();
    // Sets the control parameterized value to v. Thread safe, non-blocking.
    // Resets the control to its default value. Thread safe, non-blocking.
    void trigger();
  signals:
    void validChanged(bool valid) const;
    void groupChanged(QString group);
    void itemChanged (QString item);
    void keyChanged(ConfigKey key);
    // This signal must not connected by connect(). Use connectValueChanged()
    // instead. It will connect to the base ControlDoublePrivate as well.
    void valueChanged(double val) const;
    void defaultValueChanged(double defaultVal) const;
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
    mutable QSharedPointer<ControlDoublePrivate> m_pControl;
};
QML_DECLARE_TYPE(ControlProxy)
#endif // CONTROLPROXY_H
