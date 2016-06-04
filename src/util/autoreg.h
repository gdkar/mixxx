_Pragma("once")

#include <atomic>
#include <memory>

template<class T>
class FactoryBase {
    std::atomic<FactoryReg*> m_next{nullptr};
    static std::atomic<FactoryBase*> m_factories;
public:
    FactoryBase()
    {
        m_next.store(FactoryBase<T>::m_factories.exchange(this));
    }
    virtual ~FactoryBase() = default;
    FactoryBase<T>* next() const
    {
        return m_next.load();
    }
    virtual T* create() = 0;
};

template<class T>
/*static */ std::atomic<FactoryBase<T> *> FactoryBase<T>::m_factories{nullptr};

template<class T, class Reg>
class FactorySub : public FactorBase<Reg> {
    FactorySub()
    : FactoryBase<T>()
    { }
    virtual Reg *create() const override
    {
        return new T{};
    }
}
template<class T, class Reg>
class AutoRegister : public T {
    static FactorySub<T,R>  m_factory;
  public:
    template<class... Args>
    AutoRegister(Args... args)
    : T(args...)
    { }
    template<class... Args>
    AutoRegister(Args&&... args)
    : T(std::forward<Args>(args)...)
    { }
    virtual ~AutoRegister() = default;
}
template<class T, class Reg>
/* static */ Factorysub<T,Reg> AutoRegister<T,Reg>::m_factory{};
