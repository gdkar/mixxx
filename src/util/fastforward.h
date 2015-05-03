#ifndef UTIL_FASTFORWARD_H
#define UTIL_FASTFORWARD_H

#include <qmath.h>
#include <qatomic.h>
#include <qobject.h>
#include <qglobal.h>
#include <qsharedpointer.h>
#include <cstdlib>
#include <cstring>
template < typename T, int count >
class FastFwdPtrQ{
  static const int kCachelineSize = 64;
  static const int kCachelineSizeVoidStar = kCachelineSize/sizeof(void*);
  QAtomicPointer<T>           data[count] __attribute__((aligned(kCachelinSize)));
  qint64                      wptr        __attribute__((aligned(kCachelineSize)));
  qint64                      rptr        __attribute__((aligned(kCachelineSize)));
public:
  explicit FastFwdPtrQ()
  : wptr(0),
  , rptr(0){
    std::memset(data,0,sizeof(data));
  };
  virtual ~FastFwdPtrQ(){}
  virtual bool put(const T *item){
    qint64 wpos = wptr;
    if(!data[wpos].load()){
      data[wpos].store(item);
      wptr+= (wpos>=(count-1))?(1-count):1;
      return true;
    }else{return false;}
  }
  virtual bool get(T **item){
    qint64 rpos = rptr;
    T* addr;
    if(*item=data[rpos].load()){
      data[rpos].addr.store(0);
      rptr+= (rpos>=(count-1))?(1-count):1;
      return true;
    }else{return false;}
  }
};
#endif /* UTIL_FASTFORWARD_H */
