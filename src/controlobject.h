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

_Pragma("once")
#include <QObject>
#include <QEvent>
#include <QMutex>
#include <QSharedPointer>
#include "configobject.h"
class ControlDoublePrivate;
class ControlObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged);
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged);
    Q_PROPERTY(QString group READ group CONSTANT);
    Q_PROPERTY(QString item READ item CONSTANT);
    Q_PROPERTY(double value READ get WRITE set RESET reset NOTIFY valueChanged);
    Q_PROPERTY(double parameter READ getParameter WRITE setParameter RESET reset NOTIFY parameterChanged);
    Q_PROPERTY(double defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged);
  public:
    ControlObject(QObject * = nullptr);
    ControlObject(ConfigKey key, QObject *parent = nullptr, bool bTrack=false, bool bPersist=false);
    virtual ~ControlObject();
    // Returns a pointer to the ControlObject matching the given ConfigKey
    static ControlObject* getControl(ConfigKey key, bool warn = true);
    static ControlObject* getControl( QString group, QString item, bool warn = true);
    static ControlObject* getControl(const char* group, const char* item, bool warn = true);
    QString name() const;
    void setName(QString );
    QString description() const;
    void setDescription(QString);
    QString group() const;
    QString item() const;
    // Return the key of the object
    ConfigKey getKey() const;
    // Returns the value of the ControlObject
    virtual double get() const;
    // Returns the bool interpretation of the ControlObject
    virtual bool toBool() const;
    // Instantly returns the value of the ControlObject
    static double get(ConfigKey key);
    // Sets the ControlObject value. May require confirmation by owner.
    virtual void set(double value);
    // Sets the ControlObject value and confirms it.
    virtual void setAndConfirm(double value);
    // Instantly sets the value of the ControlObject
    static void set(ConfigKey key, double value);
    // Sets the default value
    virtual void reset();
    virtual void setDefaultValue(double dValue);
    virtual double defaultValue() const;
    virtual operator bool()const;
    virtual bool operator!() const;
    virtual operator int() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    virtual double getParameter() const;
    // Returns the parameterized value of the object. Thread safe, non-blocking.
    virtual double getParameterForValue(double value) const;
    // Sets the control parameterized value to v. Thread safe, non-blocking.
    virtual void setParameter(double v);
    // Connects a Qt slot to a signal that is delivered when a new value change
    // request arrives for this control.
    // Qt::AutoConnection: Qt ensures that the signal slot is called from the
    // thread where the receiving object was created
    // You need to use Qt::DirectConnection for the engine objects, since the
    // audio thread has no Qt event queue. But be a ware of race conditions in this case.
    // ref: http://qt-project.org/doc/qt-4.8/qt.html#ConnectionType-enum
    bool connectValueChangeRequest(const QObject* receiver, const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    bool connectValueChanged(const QObject* receiver,const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    bool connectValueChanged(const char* method, Qt::ConnectionType type = Qt::AutoConnection);
    virtual void initialize(ConfigKey key, bool bTrack, bool bPersist);
  signals:
    void valueChanged(double);
    void defaultValueChanged(double);
    void nameChanged(QString);
    void descriptionChanged(QString);
    void parameterChanged();
  protected:
    // Key of the object
    ConfigKey m_key;
    QSharedPointer<ControlDoublePrivate> m_pControl;
  private:
    bool ignoreNops() const;
};
