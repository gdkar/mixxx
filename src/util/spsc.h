#ifndef UTIL_SPSC_H
#define UTIL_SPSC_H

#include <qatomic.h>
#include <qsharedpointer.h>
#include <qthread.h>
#include <qmath.h>

namespace Mixxx{

static const int longxCacheLine = 64/sizeof(long);

class SWSR_Ptr_Buffer{
  QAtomicInteger<quintptr>      pread;
  long                          pread_padding[longxCcheLine-1];
  QAtomicInteger<quintptr>      pwrite;
  long                          pwrite_padding[longxCcheLine-1];
  const quintptr                size;
  void                        **buf;
public:
  SWSR_Ptr_Buffer( quintptr n, const bool=true)
    :pread(0)
    ,pwrite(0)
    ,size(n)
    ,buf(0){
      (void)pread_padding;
      (void)pwrite_padding;
  }
 ~SWSR_Ptr_Buffer(){
    delete[] buf;
  }
  bool init(){
    if(buf||(size==0))return false;
    buf = new (void*)[size];
    if(!buf) return false;
    reset();
    return true;
  }
  inline bool empty(){return (buf[pread.load()]==0);}
  inline bool available(){return (buf[pwrite.load()]==0);}
  inline quintptr buffersize() const { return size;}
  inline bool push ( void * const data){
    quintptr p = pwrite.load();
    if(buf[p]==0){
      buf[p] = data;
      pwrite += (p+1>=size)?(1-size):1;
      return true;
    }
    return false;
  }
  inline bool pop ( void **data){
    if(!data || empty())return false;
    quintptr p = pread.load();
    *data  = buf[p];
    buf[p] = 0;
    pread += (p+1>=size)?(1-size):1;
    return true;
  }
  inline void reset(){
    pread.store(0);
    pwrite.store(0);
    std::memset(buf,0,size*sizeof(void*));
  }
  inline quintptr length() const{
    qiintptr tread = pread.load(), twrite=pwrite.load();
    qintptr  len   = twrite-tread;
    if(len>0) return static_cast<quintptr>(len);
    if(len<0) return static_cast<quintptr>(size-len);
    if(buf[twrite]==0) return 0;
    return size;
  }
};


};
#endif /* UTIL_SPSC_H */
