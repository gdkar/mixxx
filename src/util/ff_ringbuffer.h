#ifndef UTIL_FF_RINGBUFFER_H
#define UTIL_FF_RINGBUFFER_H

#include <qmath.h>
#include <qglobal.h>
#include <qatomic.h>
#include <qsharedpointer.h>
#include <qthread.h>
#include <qthreadstorage.h>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <cstdarg>

template<typename T, int SIZE>
class FFPtrBuffer {
  QAtomicPointer<T>     m_data[SIZE];
  char                  _m_rpos_pad[64-sizeof(int)];
  int                   m_rpos;
  char                  _m__pos_pad[64-sizeof(int)];
  int                   m_wpos;
  qint64                m_wpos_count;
public:
  explicit FFPtrBuffer(): m_rpos(0), m_wpos(0),m_wpos_count(0){std::memset(reinterpret_cast<void*>(m_data),0,sizeof(m_data));}
  virtual ~FFPtrBuffer(){}
  virtual bool read ( T *&t){
    int rpos = m_rpos;
    t = m_data[rpos].loadAcquire();
    if(t){
      m_data[rpos].storeRelease(0);  
      rpos ++;
      rpos -= (rpos>=SIZE)?SIZE:0;
      m_rpos = rpos;
      return true;
    }
    return false;
  }
  virtual qint64 count() const{return m_wpos_count;}
  virtual bool write(T *&t){
    int wpos = m_wpos;
    if(!m_data[wpos].loadAcquire()){
      m_data[wpos].storeRelease(t);
      wpos ++;
      wpos -= (wpos>=SIZE)?SIZE:0;
      m_wpos = wpos;
      m_wpos_count ++;
      return true;
    }
    return false;
  }
};
template<typename T, int SIZE>
class FFItemBuffer {
  struct BufferItem{
    QAtomicInteger<int>   inuse;
    T                     item;
  };
  BufferItem              m_data[SIZE];
  char                    _m_rpos_pad[64-sizeof(int)];
  int                     m_rpos;
  char                    _m_wpos_pad[64-sizeof(int)];
  int                     m_wpos;
  qint64                  m_wpos_count;
public:
  explicit FFItemBuffer():m_rpos(0),m_wpos(0),m_wpos_count(0){std::memset(reinterpret_cast<void*>(m_data),0,sizeof(m_data));}
  virtual ~FFItemBuffer(){}
  virtual bool read(T &t){
    int is_inuse = m_data[m_rpos].inuse.loadAcquire();
    if(is_inuse){
      t = m_data[m_rpos].item;
      m_data[m_rpos].inuse.storeRelease(0);
      m_rpos = (m_rpos+1)%SIZE;
      return true;
    }else{
      return false;
    }
  }
  virtual qint64 count() const{return m_wpos_count;}
  virtual bool write(const T &t){
    int is_inuse= m_data[m_wpos].inuse.loadAcquire();
    if(!is_inuse){
      m_data[m_wpos].item = t;
      m_data[m_wpos].inuse.storeRelease(-1);
      m_wpos = (m_wpos+1)%SIZE;
      m_wpos_count++;
      return true;
    }else{
      return false;
    }
  }

};
#endif
