#ifndef UTIL_STATIC_INIT_H
#define UTIL_STATIC_INIT_H

namespace Mixxx{
  typedef void (*VoidFunc)();
  typedef void *(*PthreadFunc)(void *);

class MixxxStaticInitializer {
  const char *    m_name;
  const VoidFunc  m_ctor;
  const VoidFunc  m_dtor;
public:
  MixxxStaticInitializer(const char *name, const VoidFunc ctor, const VoidFunc dtor)
    :m_name(name), m_ctor(ctor), m_dtor(dtor) {
      qDebug() << "Running the staticly registered initializer for " << m_name << "\n";
      if(m_ctor)(m_ctor)();
    }
 ~MixxxStaticInitializer(){
    qDebug() << "Running the statically registered destructor for " << m_name << "\n";
    if(m_dtor)(m_dtor());
 }
};
};
#endif
