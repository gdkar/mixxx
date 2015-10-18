_Pragma("once")
#include <QtDebug>
#include <memory>
#include <atomic>
template<class T>
class Singleton {
  public:
    static T* create() {return instance();}
    static T* instance() { return get_instance().load();}
    static void destroy() {auto old = get_instance().exchange(nullptr); if(old)delete old;}
    virtual ~Singleton() = default;
  protected:
    static std::atomic<T*>& get_instance()
    {
      static std::atomic<T*> m_instance{new T()};
      return m_instance;
    }
    Singleton() = default;
    friend class std::unique_ptr<Singleton<T> >;
    // hide copy constructor and assign operator
    Singleton(const Singleton&) = delete;
    const Singleton& operator= (const Singleton&) = delete;
};
