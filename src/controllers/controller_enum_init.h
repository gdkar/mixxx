#ifndef CONTROLLERS_CONTROLLER_ENUM_INIT_H
#define CONTROLLERS_CONTROLLER_ENUM_INIT_H
#include "controllers/controllermanager.h"
typedef ControllerEnumerator *(*ControllerEnumeratorCtor)();
class ControllerEnumLinker {
  bool inserted;
public:
  ControllerEnumLinker(ControllerEnumeratorCtor  Ctor)
  : inserted(false){
    ControllerEnumerator * enumerator = Ctor();
    inserted = ControllerManager::appendEnumerator(enumerator);
  }
 ~ControllerEnumLinker(){}
  bool isInitialized() const{return inserted;}
};

#endif
