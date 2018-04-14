#ifndef SINGLETON_H
#define SINGLETON_H

#include <QtDebug>

template<class T>
class Singleton {
  public:
    static T* create() {
        if (!m_instance) {
            m_instance = new T();
        }
        return m_instance;
    }

    static T* instance() {
        if (!m_instance) {
            qWarning() << "Singleton class has not been created yet, returning NULL";
            m_instance = new T();
        }
        return m_instance;
    }

    static void destroy() {
        if (m_instance) {
            delete m_instance;
            m_instance = nullptr;
        }
    }

  protected:
    Singleton() = default;
    virtual ~Singleton() = default;

  private:
    // hide copy constructor and assign operator
    Singleton(const Singleton&) = delete;
    const Singleton& operator= (const Singleton&) = delete;

    static T* m_instance;
};

template<class T> T* Singleton<T>::m_instance = nullptr;

#endif // SINGLETON_H
