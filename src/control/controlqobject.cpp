#include "control/controlqobject.h"

ControlQObject::ControlQObject(QObject *parent)
:QObject(parent)
{
  connect(this,&ControlQObject::parameterRequest,
          this,&ControlQObject::setParameter,
          Qt::QueuedConnection);
}

ControlQObject::~ControlQObject()
{}
double  ControlQObject::value()const{return m_value.getValue();}
double  ControlQObject::defaultValue()const{return m_defaultValue.getValue();}
double  ControlQObject::parameter()const{return m_parameter.getValue();}
QString ControlQObject::name()const{return m_name.getValue();}
QString ControlQObject::description()const{return m_description.getValue();}
QJSValue &ControlQObject::thisObject(){return m_thisObject;}
QJSValue &ControlQObject::behavior(){return m_behavior;}
void    ControlQObject::setThisObject(QJSValue &thisObject_){
  if(!thisObject_.equals(m_thisObject)){
    m_thisObject = thisObject_;
    emit(thisObjectChanged(thisObject_));
  }
}
void    ControlQObject::setBehavior(QJSValue &behavior_){
  if(!behavior_.equals(m_behavior) && ((!behavior_.isError() && behavior_.isCallable())||(behavior_.isUndefined()))){
    m_behavior = behavior_;
    emit(behaviorChanged(m_behavior));
  }
}
void    ControlQObject::setBehavior(QJSValue &thisObject_, QJSValue &behavior_){
  bool thisObjectChanged_ = !thisObject_.equals(m_thisObject);
  bool behaviorChanged_   = false;
  if(!behavior_.equals( m_behavior) && ((!behavior_.isError() && behavior_.isCallable())||(behavior_.isUndefined()))){
    m_behavior = behavior_;
    behaviorChanged_ = true;
  }
  if(thisObjectChanged_){
    m_thisObject = thisObject_;
    emit(thisObjectChanged(m_thisObject));
  }
  if(behaviorChanged_)
    emit(behaviorChanged(m_behavior));
    
}
void    ControlQObject::setValue(double v){
  if(v!=value()){
    m_value.setValue(v);
    emit(valueChanged(v));
  }
}
void ControlQObject::setParameterRequest(double v){
  emit(parameterRequest(v));
}
void ControlQObject::setParameter(double v){
  if(v!=parameter()){
    m_parameter.setValue(v);
    emit(parameterChanged(v));
    double _v = v;
    if(!m_behavior.isError() && m_behavior.isCallable()){
      QJSValueList args;
      args << v;
      QJSValue jsv = m_behavior.callWithInstance(m_thisObject,args);
      if(jsv.isError() || !jsv.isNumber())
        return;
      _v = jsv.toNumber();
    }
    setValue(_v);
  }
}
void    ControlQObject::setDefaultValue(double v){
  if(v!=defaultValue()){
    m_defaultValue.setValue(v);
    emit(defaultValueChanged(v));
  }
}
void    ControlQObject::setName(QString v){
  if(v!=name()){
    m_name.setValue(v);
    emit(nameChanged(v));
  }
}
void    ControlQObject::setDescription(QString v){
  if(v!=description()){
    m_description.setValue(v);
    emit(descriptionChanged(v));
  }
}
void    ControlQObject::reset(){
  QJSValue _reset = m_thisObject.property("reset");
  if(_reset.isCallable()){
    QJSValueList args;
    args << defaultValue();
    QJSValue jsv = _reset.callWithInstance(m_thisObject,args);
    if(jsv.isNumber()){
      setValue(jsv.toNumber());
      return;
    }
  }
  setValue(defaultValue());
}
