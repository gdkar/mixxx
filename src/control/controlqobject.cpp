#include "control/controlqobject.h"

ControlQObject::ControlQObject(QJSEngine *engine,QJSValue ctx, QObject *parent)
:QObject(parent?parent:engine),
 m_engine(engine),
 m_context(ctx.isObject()?ctx:m_engine->globalObject()){
  m_thisObject = m_engine->toScriptValue(this);
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
    if(!name().isEmpty()){
      QJSValue old_object = m_context.property(name());
      if(old_object.equals(m_thisObject))
        m_context.setProperty(name(),QJSValue());
    }
    m_name.setValue(v);
    m_context.setProperty(name(),m_thisObject);
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
