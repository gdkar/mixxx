#ifndef SINGLETON_H
#define SINGLETON_H

#include <QtDebug>

template<class T>
class Singleton {
  public:
    static T* create() {
      return instance();
    }
    static T* instance() {
      static T* l_instance = new T();
      return l_instance;
    }
    static void destroy() {
        T * ll_instance = instance();
        delete ll_instance;
    }
  protected:
    Singleton() {}
    virtual ~Singleton() {}

  private:
    // hide copy constructor and assign operator
    Singleton(const Singleton&) {}
    const Singleton& operator= (const Singleton&) {}
};



#endif // SINGLETON_H
