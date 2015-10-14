_Pragma("once")
#include <limits>

#include <atomic>
#include <memory>
#include <QAtomicInt>
#include <QAtomicInteger>
#include <QSharedPointer>
#include <QObject>

#include "util/compatibility.h"
#include "util/assert.h"

// shared pointer based implementation for all Types sizeof(T) > sizeof(void*)

// An implementation of ControlValueAtomicBase for non-atomic types T. Uses a
// QSharedPointer<T> to reference the most recent value, and takes a reference
// ( i.e., a QSharedPointer<T> ) on reading --- thus avoiding attempting to 
// custom re-implement atomic / wait-free reference counting --- given that the
// QSharedPointer implementation, there, will be wait-free if the primitives we
// were using --- above --- are.
//
// relies on wait-free allocation of ( at least a small number of ) items of size
// sizeof(T), but honestly if we can't guarantee that we're all kinds of fucked
// anyway. reads are always consistent and wait-free if t::operator delete is.

template<typename T, bool ATOMIC = false>
class ControlValueAtomicBase
{
  public:
    ControlValueAtomicBase()=default;
    ControlValueAtomicBase(const T &val)
    {
      setValue(val);
    }
    virtual ~ControlValueAtomicBase() = default;
    T getValue() const
    {
        auto data = QSharedPointer<T>(m_data);
        if(data)
        {
          auto value = *data;
          return value;
        }
        return T{};
    }
    void setValue(const T& value)
    {
        /* requires T to have a copy assignment operator, but whatever */
        auto data = QSharedPointer<T>::create(value);
        /* atomically replace the shared pointer contents accessible by
         * new readers. current readers will have a reference to the old
         * value, so we can't reclaim it until they all drop their 
         * QSharedPointer<T>'s, at which point the value will be deleted
         * automatically. */
        m_data.swap(data);
  }
  void swap(T &other)
  {
    auto data = QSharedPointer<T>::create(other);
    m_data.swap(data);
    if(data) other = *data;
    else     other = T();
  }
  protected:
  private:
    QSharedPointer<T>   m_data;
};
template<typename T>
class ControlValueAtomicBase<T, true>
{
  public:
    ControlValueAtomicBase() = default;
    ControlValueAtomicBase( const T& val)
             {setValue(val);}
    virtual ~ControlValueAtomicBase()=default;
    T getValue() const
    {
        return m_data.load();
    }
    void setValue(const T& value)
    {
        m_data.store(value);
    }
    void swap(T & value)
    {
        value = m_data.exchange ( value );
    }
  private:
    std::atomic < T > m_data __attribute__((aligned(sizeof(T))));
};
// ControlValueAtomic is a wrapper around ControlValueAtomicBase which uses the
// sizeof(T) to determine which underlying implementation of
// ControlValueAtomicBase to use. For types where sizeof(T) <= sizeof(void*),
// the specialized implementation of ControlValueAtomicBase for types that are
// atomic on the architecture is used.
template<typename T>
class ControlValueAtomic
      :  public ControlValueAtomicBase<T, sizeof(T) <= sizeof(long double)> {
    public:
    ControlValueAtomic()
        : ControlValueAtomicBase<T, sizeof(T) <= sizeof(long double )> (){
    }
};
template<typename T>
void swap(ControlValueAtomic<T> &lhs, T&rhs)
{
  lhs.swap(rhs);
}
template<typename T>
void swap(T &lhs, ControlValueAtomic<T>&rhs)
{
  lhs.swap(rhs);
}
