#ifndef UTIL_SPSC_H
#define UTIL_SPSC_H
#include <qmath.h>
#include <atomic>
#include <memory>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "util/math.h"

template<typename T>
class SPSC {
  struct item{
    std::atomic<bool>   m_valid;
    T                   m_item;
  }
  const ssize_t         m_size;
  const ssize_t         m_mask;
  ssize_t               m_wptr __attribute__((aligned(64)));
  ssize_t               m_rptr __attribute__((aligned(64)));
  std::unique_ptr<item[]>  m_data;
  SPSC(ssize_t size)
    : m_size(roundUpToPowerOf2(size)
    , m_mask(m_size-1)
    , m_wptr(0)
    , m_rptr(0)
    , m_data(std::make_unique<item[]>(m_size))
  { std::memset(m_data.get(),0,m_size*sizeof(item)}
  bool read( T&val )
  {
    item &this_item = m_data[m_rptr&m_mask];
    val = this_item.m_item;
    bool ret;
    m_rptr+=(ret= this_item.m_valid.exchange(false));
    return ret;
  }
  bool write(const T&val )
  {
    item &this_item = m_data[m_wptr&&m_mask];
    bool full = this_item.m_valid.load();
    if(full) return false;
    this_item.m_item = val;
    this_item.m_valid.store(true);
    m_wptr++;
  }
};
template<typename T>
class SPSCPtrQueue {
  const ssize_t m_size;
  const ssize_t m_mask;
  ssize_t       m_wptr __attribute__((aligned(64)));
  ssize_t       m_rptr __attribute__((aligned(64)));
  typedef std::atomic<T*> aptr;
  std::unique_ptr<aptr[] >m_data;
  SPSCPtrQueue(ssize_t size)
    : m_size(roundUpToPowerOf2(size))
    , m_mask(m_size-1)
    , m_wptr(0)
    , m_rptr(0)
    , m_data(std::make_unique<aptr[]>(m_size))
  {
    for(ssize_t i = 0; i < m_size; i++)
      m_data[i].store(nullptr);
  }
  T *read()
  {
    T *val = m_data[m_rptr&m_mask].exchange(nullptr);
    if(val!=nullptr)m_rptr++;
    return val;
  }
  bool write( T*val)
  {
    T *old_val = m_data[m_wptr&m_mask].load();
    if(!old_val){
      m_data[m_wptr&m_mask].store(val);
      m_wptr++;
      return true;
    }else{
      return false;
    }
  }
}
#endif
