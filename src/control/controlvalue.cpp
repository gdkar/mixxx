#include "control/controlvalue.h"

ControlValueDouble::ControlValueDouble(double default_value, const QString &_name, QObject *pParent)
:QObject(pParent)
,m_name(_name){
  if(!_name.isNull())setObjectName(_name);
  setDefaultValue(default_value);
  setValue(defaultValue());
}
ControlValueDouble::ControlValueDouble(const QString&_name, QObject *pParent)
:QObject(pParent)
,m_name(_name){
  if(!_name.isNull())setObjectName(_name);
  setDefaultValue(0);
  setValue(defaultValue());
}
ControlValueDouble::~ControlValueDouble(){}
