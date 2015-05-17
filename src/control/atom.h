#ifndef CONTROL_ATOM_H
#define CONTROL_ATOM_H

#include "control/controlvalue.h"
#include <QtQml>
#include <QtQuick>
#include <QJSValue>
#include <QJSValueList>
class CVAtom : public QObject{
  Q_OBJECT;
  Q_PROPERTY(double value READ getValue WRITE setValue RESET resetValue NOTIFY valueChanged);
  Q_PROPERTY(double parameter READ parameter WRITE setParameter RESET resetValue NOTIFY valueChanged );
  Q_PROPERTY(QJSValue behavior READ behavior );
  Q_PROPERTY(QJSValue thisobj  READ thisobj );
public:
  explicit CVAtom(const QString &name="cv",QObject *pParent=0):QObject(pParent){ setObjectName(name);}
  explicit CVAtom(QJSValue thisobj, QJSValue behavior = QJSValue(), const QString&name="cv", QObject *pParent=0)
   :QObject(pParent),m_this(thisobj), m_behavior(behavior){
    setObjectName(name);
    if(!behavior.isCallable() ){
      if(thisobj.isCallable() ){
        m_behavior = thisobj;
        }else{
          if(!(m_behavior=thisobj.property("set")).isCallable()
          && !(m_behavior=thisobj.property("setValue")).isCallable()
          && !(m_behavior=thisobj.property("setParameter")).isCallable())
          m_behavior =QJSValue();
        }
      } 
    } 
  virtual ~CVAtom();
  virtual double getValue() const{ return m_value.getValue();}
  virtual void   setValue (double v){
    if(v!=m_value.getValue()){
      m_value.setValue(v);
      valueChanged(v);
    }
  }
  virtual void   resetValue(){
    if(m_reset.getValue()!=m_value.getValue()){
        m_value.setValue(m_reset.getValue());
      valueChanged(m_value.getValue());
    }
  }
  virtual double parameter() const{return m_param.getValue();}
  virtual void   setParameter(double v){
    if(m_param.getValue()!=v){
      m_param.setValue(v);
      if(m_behavior.isCallable()){
        QJSValueList args;
        args << QJSValue(v);
        QJSValue ret = m_behavior.callWithInstance(m_this,args);
        if((!ret.isError()) && (ret.isNumber())){
          setValue(ret.toNumber());
          return;
        }
      }
        setValue(v);
    }
  }
  QJSValue behavior(){return m_behavior;}
  QJSValue thisobj() {return m_this;}
signals:
  void valueChanged(double);
  void thisobjChanged(QJSValue&);
  void behaviorChanged(QJSValue&);
private:
  QJSValue      m_this;
  QJSValue      m_behavior;
  ControlValueAtomic<double>      m_value;
  ControlValueAtomic<double>      m_param;
  ControlValueAtomic<double>      m_reset;
};


typedef QSharedPointer<CVAtom> CVPtr;
#endif
