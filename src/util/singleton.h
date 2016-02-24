_Pragma("once")
#include <QtDebug>

template<class T>
class Singleton {
  public:
    static T* instance()
    {
        static T _instance{};
        return &_instance;
    }
  protected:
    Singleton() = default;
    virtual ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};
