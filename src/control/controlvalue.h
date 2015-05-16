#ifndef CONTROLVALUE_H
#define CONTROLVALUE_H

#include <limits>

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
class ControlValueAtomicBase {
  public:
    ControlValueAtomicBase(){}
    ControlValueAtomicBase(const T& t){setValue(t);}
    inline T getValue() const {
        T value;
        QSharedPointer<T> data(m_data);
        if(data) value = *data;
        return value;
    }
    inline void setValue(const T& value) {
        /* requires T to have a copy assignment operator, but whatever */
        QSharedPointer<T> data = QSharedPointer<T>::create(value);
        /* atomically replace the shared pointer contents accessible by
         * new readers. current readers will have a reference to the old
         * value, so we can't reclaim it until they all drop their 
         * QSharedPointer<T>'s, at which point the value will be deleted
         * automatically. */
        m_data.swap(data);
  }
  inline void swap(T &other){
    QSharedPointer<T> data(new T);
    *data = other;
    m_data.swap(data);
    other = (data)? *data:T();
  }
  protected:
  private:
    QSharedPointer<T>   m_data;
};
// Specialized template for types that are deemed to be atomic on the target
// architecture. Instead of using a read/write ring to guarantee atomicity,
// direct assignment/read of an aligned member variable is used.
template<typename T>
class ControlValueAtomicBase<T, true>{
  typedef typename QIntegerForSize<sizeof(T)>::Unsigned uint_type;
  public:
    ControlValueAtomicBase(){setValue(T());}
    inline T getValue() const {
        uint_type as_int = m_int.load(); 
        const T * int_ptr = reinterpret_cast<T*>(&as_int);
        return *int_ptr;
    }
    inline void setValue(const T& value) {
        uint_type new_val;
        const uint_type *int_ptr = reinterpret_cast<const uint_type*>(&value);
        new_val=*int_ptr;
        m_int.store(new_val);
    }
    inline void swap(T & value){
        uint_type new_val;
        const uint_type *int_ptr = reinterpret_cast<const uint_type *>(&value);
        new_val = *int_ptr;
        new_val = m_int.fetchAndStoreOrdered(new_val);
        const T *t_ptr = reinterpret_cast<T*>(&new_val);
        value = *t_ptr;
    }
  private:
    QAtomicInteger<uint_type> m_int;
};
// ControlValueAtomic is a wrapper around ControlValueAtomicBase which uses the
// sizeof(T) to determine which underlying implementation of
// ControlValueAtomicBase to use. For types where sizeof(T) <= sizeof(void*),
// the specialized implementation of ControlValueAtomicBase for types that are
// atomic on the architecture is used.
template<typename T>
class ControlValueAtomic :  public ControlValueAtomicBase<T, sizeof(T) <= sizeof(quintptr)> {
    public:
    ControlValueAtomic() : ControlValueAtomicBase<T, sizeof(T) <= sizeof(quintptr)> (){}
    ControlValueAtomic(const T& t):ControlValueAtomicBase<T,sizeof(T)<=sizeof(quintptr)>(t){}
};
template<typename T>
inline void swap(ControlValueAtomic<T> &lhs, T&rhs){lhs.swap(rhs);}
template<typename T>
inline void swap(T &lhs, ControlValueAtomic<T>&rhs){lhs.swap(rhs);}
#endif /* CONTROLVALUE_H */
