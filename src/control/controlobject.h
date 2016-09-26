/***************************************************************************
                          controlobject.h  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CONTROLOBJECT_H
#define CONTROLOBJECT_H

#include <QObject>
#include <QEvent>
#include <QMutex>
#include <QtQml>
#include "preferences/usersettings.h"
#include "controllers/midi/midimessage.h"
#include "control/control.h"

class ControlObject : public QObject, public QEnableSharedFromThis<ControlObject> {
    Q_OBJECT
    Q_PROPERTY(ConfigKey key READ getKey CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(double value READ get WRITE set RESET reset NOTIFY valueChanged)
    Q_PROPERTY(double defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged)
  public:
    Q_INVOKABLE ControlObject(QObject *p = nullptr);
    // bIgnoreNops: Don't emit a signal if the CO is set to its current value.
    // bTrack: Record statistics about this control.
    // bPersist: Store value on exit, load on startup.
    ControlObject(ConfigKey key, QObject *pParent,
                  bool bIgnoreNops=true, bool bTrack=false,
                  bool bPersist=false);
    virtual ~ControlObject();
    // Returns a pointer to the ControlObject matching the given ConfigKey
    static ControlObject* getControl(ConfigKey key, bool warn = false);
    static ControlObject* getControl(QString group, QString item, bool warn = false);
    static ControlObject* getControl(const char* group, const char* item, bool warn = false);

    virtual double operator += (double incr);
    virtual double operator -= (double incr);
    virtual double operator ++ ();
    virtual double operator -- ();
    virtual double operator ++ (int);
    virtual double operator -- (int);
    Q_INVOKABLE virtual double fetch_add(double val);
    Q_INVOKABLE virtual double fetch_sub(double val);
    Q_INVOKABLE virtual double exchange (double with);
    Q_INVOKABLE virtual bool   compare_exchange(double &expected, double desired);
    Q_INVOKABLE virtual double fetch_mul(double by);
    Q_INVOKABLE virtual double fetch_div(double by);
    Q_INVOKABLE virtual double fetch_toggle();

  public slots:
    QString name() const;
    void setName(QString name);
    QString description() const;
    void setDescription(QString description);
    // Return the key of the object
    ConfigKey getKey() const;
    // Returns the value of the ControlObject
    double get() const;
    // Returns the bool interpretation of the ControlObject
    bool toBool() const;
    // Instantly returns the value of the ControlObject
    static double get(ConfigKey key);
    // Sets the ControlObject value. May require confirmation by owner.
    void set(double value);
    // Sets the ControlObject value and confirms it.
    void setAndConfirm(double value);
    // Instantly sets the value of the ControlObject
    static void set(ConfigKey key, double value);
    // Sets the default value
    void reset();
    void setDefaultValue(double dValue);
    double defaultValue() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    virtual double getParameter() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    virtual double getParameterForValue(double value) const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    virtual double getParameterForMidiValue(double midiValue) const;
    // Sets the control parameterized value to v. Thread safe, non-blocking.
    virtual void setParameter(double v);
    // Sets the control parameterized value to v. Thread safe, non-blocking.
    virtual void setParameterFrom(double v, QObject* pSender = nullptr);
    virtual double increment(double x);
    virtual double decrement(double x);
    // Connects a Qt slot to a signal that is delivered when a new value change
    // request arrives for this control.
    // Qt::AutoConnection: Qt ensures that the signal slot is called from the
    // thread where the receiving object was created
    // You need to use Qt::DirectConnection for the engine objects, since the
    // audio thread has no Qt event queue. But be a ware of race conditions in this case.
    // ref: http://qt-project.org/doc/qt-4.8/qt.html#ConnectionType-enum
    bool connectValueChangeRequest(const QObject* receiver,
                                   const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    virtual void trigger();
  signals:
    void nameChanged();
    void descriptionChanged();
    void valueChanged(double);
    void defaultValueChanged(double);
    void valueChangedFromEngine(double);
    void triggered();
  public:
    // DEPRECATED: Called to set the control value from the controller
    // subsystem.
    virtual void setValueFromMidi(MidiOpCode o, double v);
    virtual double getMidiParameter() const;
  protected:
    // Key of the object
    ConfigKey m_key;
    QSharedPointer<ControlDoublePrivate> m_pControl;
    QWeakPointer  <ControlDoublePrivate> m_wControl;
  private slots:
    void privateValueChanged(double value, QObject* pSetter);
  private:
    QSharedPointer<ControlDoublePrivate> control() const;
    void initialize(ConfigKey key, bool bIgnoreNops, bool bTrack,bool bPersist);
    bool ignoreNops() const;
};
QML_DECLARE_TYPE(ControlObject);
#endif
