#ifndef CONTROLVALUE_H
#define CONTROLVALUE_H

#include <limits>

#include <QAtomicInt>
#include <QAtomicInteger>
#include <QSharedPointer>
#include <QObject>

#include "util/compatibility.h"
#include "util/assert.h"

// for look free access, this value has to be >= the number of value using threads
// value must be a fraction of an integer
const int cRingSize = 8;
// there are basicly unlimited readers allowed at each ring element
// but we have to count them so max() is just fine.
// NOTE(rryan): Wrapping max with parentheses avoids conflict with the max macro
// defined in windows.h.
const int cReaderSlotCnt = (std::numeric_limits<int>::max)();

// A single instance of a value of type T along with an atomic integer which
// tracks the current number of readers or writers of the slot. The value
// m_readerSlots starts at cReaderSlotCnt and counts down to 0. If the value is
// 0 or less then reads to the value fail because there are either too many
// readers or a write is occurring. A write to the value will fail if
// m_readerSlots is not equal to cReaderSlotCnt (e.g. there is an active
// reader).
template<typename T>
class ControlRingValue {
  public:
    ControlRingValue()
        : m_value(T()),
          m_readerSlots(cReaderSlotCnt) {
    }

    bool tryGet(T* value) const {
        // Read while consuming one readerSlot
        bool hasSlot = (m_readerSlots.fetchAndAddAcquire(-1) > 0);
        if (hasSlot) {
            *value = m_value;
        }
        (void)m_readerSlots.fetchAndAddRelease(1);
        return hasSlot;
    }

    bool trySet(const T& value) {
        // try to lock this element entirely for reading
        if (m_readerSlots.testAndSetAcquire(cReaderSlotCnt, 0)) {
            m_value = value;
            m_readerSlots.fetchAndAddRelease(cReaderSlotCnt);
            return true;
        }
        return false;
   }

  private:
    T m_value;
    mutable QAtomicInt m_readerSlots;
};

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
    inline T getValue() const {
        T value;
        QSharedPointer<T> data(m_data);
        if(data)
          value = *data;
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
    if(data)
      other = *data;
    else
      other = T();
  }

  protected:
    ControlValueAtomicBase(){
    }

  private:
    QSharedPointer<T>   m_data;
};

// Specialized template for types that are deemed to be atomic on the target
// architecture. Instead of using a read/write ring to guarantee atomicity,
// direct assignment/read of an aligned member variable is used.
template<typename T>
class ControlValueAtomicBase<T, true>{
  public:
    ControlValueAtomicBase()
             {
      setValue(T());
    }
    inline T getValue() const {
        quintptr as_int = m_int.load(); 
        const T * int_ptr = reinterpret_cast<T*>(&as_int);
        return *int_ptr;
    }
    inline void setValue(const T& value) {
        quintptr new_val;
    switch(sizeof(T)){
      case 1:{
            const quint8 *int_ptr = reinterpret_cast<const quint8*>(&value);
            new_val = *int_ptr;
            break;
            }
      case 2:{
            const quint16 *int_ptr = reinterpret_cast<const quint16*>(&value);
            new_val = *int_ptr;
            break;
            }
      case 4:{
            const quint32 *int_ptr = reinterpret_cast<const quint32*>(&value);
            new_val = *int_ptr;
            break;
            }
      case 8:{
            const quint64 *int_ptr = reinterpret_cast<const quint64*>(&value);
            new_val = *int_ptr;
            break;
            }
        }
        m_int.store(new_val);
    }
    inline void swap(T & value){
         quintptr new_val;
    switch(sizeof(T)){
      case 1:{
            const quint8 *int_ptr = reinterpret_cast<const quint8*>(&value);
            new_val = *int_ptr;
            break;
            }
      case 2:{
            const quint16 *int_ptr = reinterpret_cast<const quint16*>(&value);
            new_val = *int_ptr;
            break;
            }
      case 4:{
            const quint32 *int_ptr = reinterpret_cast<const quint32*>(&value);
            new_val = *int_ptr;
            break;
            }
      case 8:{
            const quint64 *int_ptr = reinterpret_cast<const quint64*>(&value);
            new_val = *int_ptr;
            break;
            }
        }
        new_val = m_int.fetchAndStoreOrdered(new_val);
        const T *t_ptr = reinterpret_cast<T*>(&new_val);
        value = *t_ptr;
    }
  private:
#if 0
#if defined(__GNUC__)
    T m_value __attribute__ ((aligned(sizeof(void*))));
#elif defined(_MSC_VER)
#ifdef _WIN64
    T __declspec(align(8)) m_value;
#else
    T __declspec(align(4)) m_value;
#endif
#else
    T m_value;
#endif
#endif
    QAtomicInteger<quintptr> m_int;
};
#if 0
#include "util/compatibility.h"
#ifdef MIXXX_HAVE_ATOMIC_128
#include <stdlib.h>
template<typename T>
class ControlValueAtomicBase<T,false,true>{
  public:
    ControlValueAtomicBase(){
    }
   ~ControlValueAtomicBase(){}
  inline T getValue(){
    T value;
    MixxxAtomicInt128 other = m_int;
    memmove(reinterpret_cast<char *>(&value),reinterpret_cast<char*>(&other),sizeof(T));
    return value;
  }
  inline void  setValue(const T &value){
    MixxxAtomicInt128 other;
    memmove(reinterpret_cast<char*>(&other),reinterpret_cast<char*>(&value),sizeof(T));
    m_int = other;
  }
  inline void swap(T & value){
    MixxxAtomicInt128 other;
    memmove(reinterpret_cast<char*>(&other),reinterpret_cast<char*>(&value),sizeof(T));
    other = m_int.fetchAndStoreRelaxed(other);
    memmove(reinterpret_cast<char*>(&value),reinterpret_cast<char*>(&other),sizeof(T));
  }
  private:
  MixxxAtomicInt128 m_int;
};
#endif
#endif
// ControlValueAtomic is a wrapper around ControlValueAtomicBase which uses the
// sizeof(T) to determine which underlying implementation of
// ControlValueAtomicBase to use. For types where sizeof(T) <= sizeof(void*),
// the specialized implementation of ControlValueAtomicBase for types that are
// atomic on the architecture is used.
template<typename T>
class ControlValueAtomic
      :  public ControlValueAtomicBase<T, sizeof(T) <= sizeof(quintptr)> {
    public:
    ControlValueAtomic()
        : ControlValueAtomicBase<T, sizeof(T) <= sizeof(quintptr)> (){
    }
};
template<typename T>
inline void swap(ControlValueAtomic<T> &lhs, T&rhs){
  lhs.swap(rhs);
}
template<typename T>
inline void swap(T &lhs, ControlValueAtomic<T>&rhs){
  lhs.swap(rhs);
}
#endif /* CONTROLVALUE_H */
