#ifndef CONTROL_STATEVARIABLE_H
#define CONTROL_STATEVARIABLE_H

#include <limits>

#include <QAtomicInt>
#include <QAtomicInteger>
#include <QSharedPointer>
#include <QObject>


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
class StateVarBase{
  typedef QSharedPointer<T> atomic_pointer;
  public:
    inline T get() const {
        T value;
        atomic_pointer data(m_data);
        if(data) value = *data;
        return value;
    }
    inline void set(const T& value) {
        atomic_pointer data = atomic_pointer::create(value);
        m_data.swap(data);
  }
  inline void swap(T &other){
    atomic_pointer data = atomic_pointer::create(other);
    m_data.swap(data);
    other = (data)? *data:T();
  }
  private:
    atomic_pointer   m_data __attribute__((aligned(64)));
};

// Specialized template for types that are deemed to be atomic on the target
// architecture. Instead of using a read/write ring to guarantee atomicity,
// direct assignment/read of an aligned member variable is used.
template<typename T>
class StateVarBase<T, true>{
  typedef typename QIntegerForSize<sizeof(T)>::Unsigned uint_type;
  public:
    inline T get() const {
        uint_type as_int = m_int.load(); 
        const T * int_ptr = reinterpret_cast<T*>(&as_int);
        return *int_ptr;
    }
    inline void set(const T& value) {
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
    QAtomicInteger<uint_type> m_int __attribute__((aligned(64)));
};
// ControlValueAtomic is a wrapper around ControlValueAtomicBase which uses the
// sizeof(T) to determine which underlying implementation of
// ControlValueAtomicBase to use. For types where sizeof(T) <= sizeof(void*),
// the specialized implementation of ControlValueAtomicBase for types that are
// atomic on the architecture is used.
template<typename T>
class StateVar :  public StateVarBase<T, sizeof(T) <= sizeof(quintptr)>, public QObject {
    Q_OBJECT;
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged);
    Q_PROPERTY(QString desciption READ description WRITE setDescription NOTIFY descriptionChanged);
    Q_PROPERTY(T value READ value WRITE setValue NOTIFY valueChanged);
    public:
    explicit StateVar(QObject *pParent = 0) : StateVarBase<T, sizeof(T) <= sizeof(quintptr)> ()
    , QObject(pParent) {}
    explicit StateVar(const StateVar<T> &other):QObject(other.parent()),m_name(other.m_name),m_description(other.m_description){
      setValue(other.value()); 
      setObjectName(other.objectName());
    }
    explicit StateVar(const QString & name, const T &initial = T(),QObject *pParent = 0) : StateVarBase<T, sizeof(T) <= sizeof(quintptr)> (), QObject(pParent),m_name(name)
     {setValue(initial);}
    virtual ~StateVar(){}
    signals:
      void nameChanged(const QString &);
      void descriptionChanged(const QString &);
      void valueChanged(const T&);
    public slots:
      virtual const QString &name()const{return m_name;}
      virtual void setName(const QString&new_name){
        if(new_name!=name()){
          m_name =new_name;
          setObjectName(new_name);
          emit(nameChanged(name()));
        }
      }
      virtual const QString &description(){return m_description;}
      virtual void setDescription(const QString &new_desc){
        if(new_desc !=description()){
          m_description = new_desc;
          emit(descriptionChanged(description()));
        }
      }
      T value()const { return StateVarBase<T,sizeof(T)<=sizeof(quintptr)>::get();}
      void setValue(T val){T v(val);StateVarBase<T,sizeof(T)<=sizeof(quintptr)>::swap(v);if(v!=val)emit(valueChanged(val));}
  private:
    QString           m_name;
    QString           m_description;
};
template<typename T>
inline void swap(StateVar<T> &lhs, T&rhs){
  lhs.swap(rhs);
}
template<typename T>
inline void swap(T &lhs, StateVar<T> &rhs){
  lhs.swap(rhs);
}
#endif /* CONTROL_STATEVARIABLE_H */
