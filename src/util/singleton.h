#ifndef SINGLETON_H
#define SINGLETON_H

#include <QtDebug>
#include <qatomic.h>
#include <qsharedpointer.h>
#include <qmath.h>
template<class T>
class Singleton{
public:
    static T* create() {
        T * l_instance = m_instance.loadAcquire();
        if (Q_UNLIKELY(!l_instance)) {
          l_instance = new T();
          if(Q_UNLIKELY(!m_instance.testAndSetRelaxed(0,l_instance))){
            delete l_instance;
            l_instance = m_instance.loadAcquire();
          }
        }
        return l_instance;;
    }
    static T* instance() {return create();}
    static void destroy() {
      T *l_instance = m_instance.loadAcquire();
        if (Q_LIKELY(l_instance)) {
            if(Q_LIKELY(m_instance.testAndSetRelaxed(l_instance, 0))) delete l_instance;
        }
    }
protected:
    Singleton() {}
    virtual ~Singleton() {}
private:
    //hide copy constructor and assign operator
    Singleton(const Singleton&) {}
    const Singleton& operator= (const Singleton&) {}

    static QAtomicPointer<T> m_instance;
};

template<class T> QAtomicPointer<T> Singleton<T>::m_instance = 0;

#endif // SINGLETON_H
