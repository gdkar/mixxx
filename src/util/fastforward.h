#ifndef UTIL_FASTFORWARD_H
#define UTIL_FASTFORWARD_H

#include <qmath.h>
#include <qsharedpointer.h>
#include <qatomic.h>
#include <qobject.h>
#include <qglobal.h>
#include <cstdlib>
#include <cstring>

#include "util/fastforward.h"

template < typename T, int count >
class FastForwardQueue{
  static const int kCachelineSize = 64;
  static const int kCachelineSizeVoidStar = kCachelineSize/sizeof(void*);
  struct data_element{
    QAtomicPointer<T>         addr;
    T                         item;
  };
  data_element                data[count] __attribute__((aligned(kCachelinSize)));
  qint64                      wptr        __attribute__((aligned(kCachelineSize)));
  qint64                      rptr        __attribute__((aligned(kCachelineSize)));
public:
  explicit FastForwardQueue()
  : wptr(0),
  , rptr(0){
    std::memset(data,0,sizeof(data));
  };
  virtual ~FastForwardQueue(){}
  virtual bool put(const T &item){
    qint64 wpos = wptr;
    if(!data[wpos].addr.load()){
      data[wpos].item = item;
      data[wpos].addr.store(&data[wpos].item);
      wptr+= (wpos>=(count-1))?(1-count):1;
      return true;
    }else{
      return false;
    }
  }
  virtual bool get(T &item){
    qint64 rpos = rptr;
    T* addr;
    if((addr=data[wpos].addr.load())&&addr!=(reinterpret_cast<T*>(-1)){
      item              = data[rpos].item;
      data[rpos].addr.store(0);
      rptr+= (rpos>=(count-1))?(1-count):1;
      return true;
    }else{
      return false;
    }
  }
  virtual void flush(){
    qint64 rpos = rptr.load();
    for(int i = 0; i < count; i++){
      data[rpos].addr.store(reinterpret_cast<T*>(-1));
      rpos+= (rpos>=(count-1)?(1-count):1;
    }
    rptr.store(0);
    wptr.store(0);
    for(int i = 0; i < count; i++){
      data[i].addr.store(0);
    }
  }
};
#endif /* UTIL_FASTFORWARD_H */
