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

#include "configobject.h"
#include "controllers/midi/midimessage.h"
#include "control/control.h"

class ControlObject : public QObject {
    Q_OBJECT
  public:
    ControlObject();
    // bIgnoreNops: Don't emit a signal if the CO is set to its current value.
    // bTrack: Record statistics about this control.
    // bPersist: Store value on exit, load on startup.
    ControlObject(ConfigKey key,bool bIgnoreNops=true, bool bTrack=false, bool bPersist=false);
    virtual ~ControlObject();
    // Returns a pointer to the ControlObject matching the given ConfigKey
    static ControlObject* getControl(const ConfigKey& key, bool warn = true);
    static ControlObject* getControl(const QString& group, const QString& item, bool warn = true);
    static ControlObject* getControl(const char* group, const char* item, bool warn = true);
    QString name() const;
    void setName(const QString& name);
    QString description() const;
    void setDescription(const QString& description);
    // Return the key of the object
    ConfigKey getKey() const;
    // Returns the value of the ControlObject
    virtual double get() const;
    // Returns the bool interpretation of the ControlObject
    virtual bool toBool() const;
    // Instantly returns the value of the ControlObject
    static double get(const ConfigKey& key);
    // Sets the ControlObject value. May require confirmation by owner.
    virtual void set(double value);
    // Sets the ControlObject value and confirms it.
    virtual void setAndConfirm(double value);
    // Instantly sets the value of the ControlObject
    static void set(const ConfigKey& key, const double& value);
    // Sets the default value
    virtual void reset();
    virtual void setDefaultValue(double dValue);
    virtual double defaultValue() const;
    virtual operator bool()const;
    virtual bool operator!() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    virtual double getParameter() const;

    // Returns the parameterized value of the object. Thread safe, non-blocking.
    virtual double getParameterForValue(double value) const;

    // Sets the control parameterized value to v. Thread safe, non-blocking.
    virtual void setParameter(double v);

    // Sets the control parameterized value to v. Thread safe, non-blocking.
    virtual void setParameterFrom(double v, QObject* pSender = nullptr);

    // Connects a Qt slot to a signal that is delivered when a new value change
    // request arrives for this control.
    // Qt::AutoConnection: Qt ensures that the signal slot is called from the
    // thread where the receiving object was created
    // You need to use Qt::DirectConnection for the engine objects, since the
    // audio thread has no Qt event queue. But be a ware of race conditions in this case.
    // ref: http://qt-project.org/doc/qt-4.8/qt.html#ConnectionType-enum
    bool connectValueChangeRequest(const QObject* receiver, const char* method, Qt::ConnectionType type = Qt::AutoConnection);
  signals:
    void valueChanged(double);
    void valueChangedFromEngine(double);
  protected:
    // Key of the object
    ConfigKey m_key;
    QSharedPointer<ControlDoublePrivate> m_pControl;
  private slots:
    virtual void privateValueChanged(double value, QObject* pSetter);
  private:
    virtual void initialize(ConfigKey key, bool bIgnoreNops, bool bTrack, bool bPersist);
    bool ignoreNops() const;
};
#endif
