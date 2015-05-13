#ifndef SINGLETON_H
#define SINGLETON_H

#include <QtDebug>

template<class T>
class Singleton{
public:
    static T* create() {
        return s_singleton->m_instance;
    }

    static T* instance() {
        return s_singleton->m_instance;
    }
    static void destroy() {
    }

protected:
    Singleton():m_instance(new T()) {}
    virtual ~Singleton() {delete m_instance;}

private:
    //hide copy constructor and assign operator
    Singleton(const Singleton&) {}
    const Singleton& operator= (const Singleton&) {}
    T*                   m_instance;
    static Singleton<T>* s_singleton;
};
template<class T> Singleton<T>* Singleton<T>::s_singleton= new Singleton<T>();

#endif // SINGLETON_H
