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
          m_readerSlots(cReaderSlotCnt) {}
    bool tryGet(T* value) const {
        // Read while consuming one readerSlot
        bool hasSlot = (m_readerSlots.fetchAndAddAcquire(-1) > 0);
        if (hasSlot) {*value = m_value;}
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
    other = (data)? *data:T();
  }
  protected:
    ControlValueAtomicBase(){}
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
};
class ControlValueDouble : public QObject {
  Q_OBJECT;
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged);
  Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged);
  Q_PROPERTY(double value READ value WRITE setValue RESET resetValue NOTIFY valueChanged);
  Q_PROPERTY(double defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged);
  ControlValueAtomic<double>        m_value;
  ControlValueAtomic<double>        m_defaultValue;
  QString                           m_name;
  QString                           m_description;
public:
  explicit ControlValueDouble(double default_value, const QString &_name,QObject *pParent =0);
  explicit ControlValueDouble(const QString& _name, QObject *pParent=0);
  virtual ~ControlValueDouble();
public slots:
  virtual const QString& name()const{return m_name;}
  virtual void setName(const QString&val){if(val!=name()){m_name=val;if(!val.isNull())setObjectName(val);emit nameChanged(val);}}
  virtual const QString& description()const{return m_description;}
  virtual void setDescription(const QString&val){if(val!=description()){m_description=val;emit descriptionChanged(val);}}
  virtual double value() const{return m_value.getValue();}
  virtual void setValue(double val){if(value()!=val){m_value.setValue(val);emit valueChanged(val);}}
  virtual void resetValue(){setValue(defaultValue());}
  virtual double defaultValue() const{return m_defaultValue.getValue();}
  virtual void setDefaultValue(double val){if(val!=defaultValue()){m_defaultValue.setValue(val);emit defaultValueChanged(val);}}
signals:
    void nameChanged(const QString&);
    void descriptionChanged(const QString&);
    void valueChanged(double);
    void defaultValueChanged(double);
public:
  virtual operator bool() const{return value()!=0;}
  virtual operator double()const{return value();}
  virtual operator int()const{return static_cast<int>(value());}
  virtual operator long long() const{return static_cast<long long>(value());}
  virtual ControlValueDouble &operator =(const ControlValueDouble &other){setValue(other.value());return *this;}
  virtual ControlValueDouble &operator =(double v){setValue(v); return *this;}
  virtual ControlValueDouble &operator =(bool v){setValue(v?1.0:0.0);return *this;}
  virtual ControlValueDouble &operator =(int v){setValue(static_cast<double>(v));return *this;}
  virtual ControlValueDouble &operator =(long long v){setValue(static_cast<double>(v));return *this;}
  virtual bool operator !() const{return value()==0;}
  virtual bool operator ==(const ControlValueDouble &other){return value()==other.value();}
  virtual bool operator !=(const ControlValueDouble &other){return value()!=other.value();}
  virtual bool operator > (const ControlValueDouble &other){return value() > other.value();}
  virtual bool operator < (const ControlValueDouble &other){return value() < other.value();}
  virtual void swap(double &other){m_value.swap(other);}
  virtual void swap(int &other){double val = static_cast<double>(other);m_value.swap(val);other=static_cast<int>(val);}
  virtual void swap(bool &other){double val = other?1.0:0.0;m_value.swap(val);other = val!=0.0;}
  virtual void swap(long long &other){double val = static_cast<double>(other);m_value.swap(val);other=static_cast<long long>(val);}
};
namespace std{
  inline void swap(ControlValueDouble &lhs, double &rhs){
    lhs.swap(rhs);
  }
  inline void swap(ControlValueDouble &lhs, int &rhs){lhs.swap(rhs);}
  inline void swap(ControlValueDouble &lhs, long long &rhs){lhs.swap(rhs);}
  inline void swap(ControlValueDouble &lhs, bool &rhs){lhs.swap(rhs);}
  inline void swap(double &lhs, ControlValueDouble &rhs){rhs.swap(lhs);}
  inline void swap(int &lhs, ControlValueDouble &rhs){rhs.swap(lhs);}
  inline void swap(long long &lhs, ControlValueDouble &rhs){rhs.swap(lhs);}
  inline void swap(bool &lhs, ControlValueDouble &rhs){rhs.swap(lhs);}
  template<typename T>
  inline void swap(ControlValueAtomic<T> &lhs, T&rhs){
    lhs.swap(rhs);
  }
  template<typename T>
  inline void swap(T &lhs, ControlValueAtomic<T>&rhs){
    lhs.swap(rhs);
  }
};
#endif /* CONTROLVALUE_H */
