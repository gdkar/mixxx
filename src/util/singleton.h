#ifndef SINGLETON_H
#define SINGLETON_H

#include <QtDebug>
#include <memory>
template<class T>
class Singleton {
  public:
    static T* create() {return instance();}
    static T* instance() {
      static std::unique_ptr<T> l_instance{new T()};
        return l_instance.get();
    }
    static void destroy() {}
    Singleton(const Singleton&)=delete;
    const Singleton&operator =(const Singleton&)=delete;
    Singleton&operator =(Singleton&&)=delete;
    virtual ~Singleton() {}
  protected:
    Singleton() {}
};
#endif // SINGLETON_H
