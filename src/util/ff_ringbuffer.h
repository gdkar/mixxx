#ifndef UTIL_FF_RINGBUFFER_H
#define UTIL_FF_RINGBUFFER_H

#include <qmath.h>
#include <qthread.h>
#include <qglobal.h>
#include <qatomic.h>
#include <qsharedpointer.h>

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <cstdint>

template<typename T>
class FFPtrRingBuffer {
  QAtomicPointer<T>   *  m_data 
  qint64                 m_data_size;
  qint64                 m_offset_beg __attribute__((aligned(64)));
  qint64                 m_offset_end __attribute__((aligned(64)));
  public:
    explicit FFPtrRingBuffer(qint64 size)
      : m_data(new QAtomicPointer<T>[size])
      , m_data_size(size)
      , m_offset_beg(0)
      , m_offset_end(0){
        std::memset(reinterpret_cast<void*>(m_data),0,m_data_size*sizeof(QAtomicPointer<T>));
      }
    virtual ~FFPtrRingBuffer(){
      delete[] m_data;
    }
    T * read(){
      T * ret = m_data[m_offset_beg].loadAcquire();
      if(T){
        m_data[m_offset_beg].storeRelease(0);
        m_offset_beg ++;
        m_offset_beg -= (m_offet_beg>=m_data_size)?m_data_size:0;
      }
      return ret;
    }
    bool write(T * val){
      T * prev = m_data[m_offset_end].loadAcquire();
      if(!prev){
        m_data[m_offset_end].storeRelease(val);
        m_offset_end ++;
        m_offset_end -= (m_offset_end >= m_data_size)?m_data_size:0;
        return true;
      }
      return false;
    }
};

#endif
