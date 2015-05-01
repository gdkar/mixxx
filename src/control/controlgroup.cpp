#include <qstring.h>
#include <qstringlist.h>
#include <qbytearray.h>
#include <qvariant.h>
#include <qvariant.h>
#include <qobject.h>
#include "control/controlgroup.h"
Q_DECLARE_METATYPE(QObject*);
class ControlGroupPlaceholder : public QObject {
  
};

ControlGroup::ControlGroup(const QString &_name, QObject *pParent)
  : QObject(pParent)
  , m_name(_name){

}

ControlGroup::~ControlGroup(){

}

const QString & ControlGroup::name() const{return m_name;}
void ControlGroup::setName(const QString &_name){
  if(_name!=name()){
    m_name = _name;
    setObjectName(_name);
    emit(nameChanged(name()));
  }
}
const QString & ControlGroup::description() const{return m_description;}
void ControlGroup::setDescription(const QString &_description){
  if(_description!=description()){
    m_description = _description;
    emit(descriptionChanged(description()));
  }
}
QObject *ControlGroup::control(const QString &which){
  QStringList parts = which.split(".",QString::SkipEmptyParts);
  QObject *ret = this;
  foreach(const QString &part, parts){
    QObject *found;
    if(!(found = ret->property(part.toLocal8Bit().constData()).value<QObject*>()) && !(found = ret->findChild<QObject*>(part))) return 0;
    ret = found;
  }
  return ret;
}
void   ControlGroup::addControl(const QString &_name, QObject *obj){
   QStringList parts = _name.split(".",QString::SkipEmptyParts);
   QObject *ret = this;
   bool changed = false;
   if(!obj){
     ret = this->control(_name);
     delete ret;
     changed=true;
   }else{
    if(_name.contains(".")){
      for(int i = 0; i < parts.size()-1; i++){
        const QString &part = parts[i];
        QObject *found = ret;
        if(((found = ret->property(part.toLocal8Bit().constData()).value<QObject*>())||(found=ret->findChild<QObject*>(part)))){
          ret = found;
        }else{
          found = new ControlGroup(part,ret);
          ret->setProperty(part.toLocal8Bit().constData(),QVariant::fromValue(found));
          found = ret;
          changed = true;
        }
      }
    }
    QObject *prop = ret->property(parts.back().toLocal8Bit().constData()).value<QObject*>();
    if(prop && prop != obj){
      foreach(const QByteArray &property_name, prop->dynamicPropertyNames()){
        QObject *this_child = prop->property(property_name.constData()).value<QObject*>();
        if(this_child->parent() == prop) this_child->setParent(obj);
        obj->setProperty(property_name.constData(), QVariant::fromValue(this_child));
      }
    }if(prop!=obj){
      ret->setProperty(parts.back().toLocal8Bit().constData(), QVariant::fromValue(obj));
      obj->setParent(ret);
      changed = true;
    }
   }
   if(changed)
     emit(controlChanged(_name));
}
