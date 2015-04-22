#ifndef CONTROL_CONTROLQOBJECT_H
#define CONTROL_CONTROLQOBJECT_H

#include <QObject>
#include <QtQml>
#include <QtScript>
#include <QJSValue>
#include <QJSValueList>
#include <QJSEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlListProperty>
#include <QList>
#include <QVector>
#include <qmath.h>
#include <qsharedpointer.h>
#include <qatomic.h>
#include "control/controlvalue.h"
class ControlQObject : public QObject{
  Q_OBJECT;
  Q_PROPERTY(double value READ value WRITE setValue RESET reset NOTIFY valueChanged);
  Q_PROPERTY(double parameter READ parameter WRITE setParameter RESET reset NOTIFY parameterChanged);
  Q_PROPERTY(double defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged);
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged);
  Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged);
  Q_PROPERTY(QJSValue thisObject READ thisObject WRITE setThisObject NOTIFY thisObjectChanged);
  Q_PROPERTY(QJSValue behavior READ behavior WRITE setBehavior NOTIFY behaviorChanged);
public:
  ControlQObject(QObject *parent=0); 
  virtual ~ControlQObject();

  Q_INVOKABLE double  value()const;
  Q_INVOKABLE double  parameter()const;
  Q_INVOKABLE double  defaultValue()const;
  Q_INVOKABLE QString name()const;
  Q_INVOKABLE QString description()const;
  Q_INVOKABLE QJSValue &thisObject();
  Q_INVOKABLE QJSValue &behavior();
signals:
  void valueChanged(double);
  void parameterRequest(double);
  void parameterChanged(double);
  void defaultValueChanged(double);
  void nameChanged(QString);
  void descriptionChanged(QString);
  void thisObjectChanged(QJSValue);
  void behaviorChanged(QJSValue);
public slots:
  void    setValue(double v);
  void    setParameter(double v);
  void    setParameterRequest(double v);
  void    setDefaultValue(double v);
  void    setName(QString v);
  void    setDescription(QString v);
  void    reset();
  void    setThisObject(QJSValue &thisObject_);
  void    setBehavior(QJSValue &behavior_);
  void    setBehavior(QJSValue &thisObject_, QJSValue &behavior_);
private:
  ControlValueAtomic<double>    m_parameter;
  ControlValueAtomic<double>    m_value;
  ControlValueAtomic<double>    m_defaultValue;
  ControlValueAtomic<QString>   m_name;
  ControlValueAtomic<QString>   m_description;
  QJSValue                      m_thisObject;
  QJSValue                      m_behavior;
};

#endif
