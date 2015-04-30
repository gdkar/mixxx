#ifndef SINGLETON_H
#define SINGLETON_H

#include <QtDebug>

template<class T>
class Singleton
{
public:
    static T* create() {
        T* current = s_instance.load();
        if(Q_UNLIKELY(!current)){
          current = new T();
          if(Q_UNLIKELY(!s_instance.testAndSetRelaxed(0,current))){
            delete current;
            current = s_instance.load();
          }
        }
        return current;
    }
    static T* instance() {
//        if (s_instance == NULL) {qWarning() << "Singleton class has not been created yet, returning NULL";}
        return create();
    }
    static void destroy() {T* current = s_instance.load();
      if(Q_LIKELY(current)){
        if(Q_LIKELY(s_instance.testAndSetRelaxed(current,0)))
          delete current;
        }
    }
protected:
    Singleton() {}
    virtual ~Singleton() {}

private:
    //hide copy constructor and assign operator
    Singleton(const Singleton&) {}
    const Singleton& operator= (const Singleton&) {}
    static QAtomicPointer<T> s_instance;
};
template<class T> T* Singleton<T>::s_instance = 0;

#endif // SINGLETON_H
