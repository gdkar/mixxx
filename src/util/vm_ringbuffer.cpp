#include "util/vm_ringbuffer.h"

VMRingBuffer::VMRingBuffer(qint64 size)
  : m_data(0)
    , m_data_size(size)
    , m_offset_beg(0)
    , m_offset_end(0){
    /* race condition here, but the only value 
      * any thread might write is the same ( page size )
      * so it's benign.
      */
  if(Q_UNLIKELY(!s_page_size)){
    s_page_size = sysconf(_SC_PAGESIZE);
  }
  char *address = reinterpret_cast<char*>(MAP_FAILED);
  m_data_size = (m_data_size +(s_page_size-1))&(~(s_page_size-1));
  int fd = -1, ret = -1;
  fd = open("/dev/zero",O_RDWR);
  RELEASE_ASSERT((fd>=0));
  m_data = reinterpret_cast<char*>(mmap ( 0, m_data_size*2,PROT_NONE,
      MAP_ANONYMOUS|MAP_PRIVATE, -1, 0));
  RELEASE_ASSERT(m_data != MAP_FAILED);
  address = reinterpret_cast<char*>(mmap(reinterpret_cast<void*>(m_data), m_data_size, PROT_READ|PROT_WRITE,
      MAP_FIXED|MAP_SHARED|MAP_PRIVATE, fd, 0));
  RELEASE_ASSERT(address != MAP_FAILED);
  address = reinterpret_cast<char*>(mmap(reinterpret_cast<void*>(m_data + m_data_size), m_data_size, PROT_READ|PROT_WRITE,
      MAP_FIXED|MAP_SHARED|MAP_PRIVATE, fd, 0));
  RELEASE_ASSERT(address != reinterpret_cast<char*>(MAP_FAILED));
  ret = close(fd);
  RELEASE_ASSERT(ret==0);
}

VMRingBuffer::~VMRingBuffer(){
    if(m_data){
      munmap(m_data,2*m_data_size);
      m_data     =0;
      m_data_size=0;
    }
}
qint64 VMRingBuffer::read_space(){
  m_offset_end_save = m_offset_end.loadAcquire();
  qint64 offset_beg = m_offset_beg.loadAcquire();
  return (m_offset_end_save < offset_beg)?m_offset_end_save + m_data_size - offset_beg
      : m_offset_end_save - offset_beg;
}
qint64 VMRingBuffer::write_space(){
  m_offset_beg_save = m_offset_beg.loadAcquire();
  qint64 offset_end = m_offset_end.loadAcquire();
  return (m_offset_beg_save < offset_end)?m_offset_beg_save + m_data_size - offset_end
    : m_offset_beg_save - offset_end;
}
bool VMRingBuffer::read(char *data, qint64 size){
  qint64 offset_beg = m_offset_beg.loadAcquire();
  qint64 offset_end = m_offset_end_save;
  offset_end += ((offset_end<offset_beg)?m_data_size:0);
  if(offset_end - offset_beg < size){
    offset_end = m_offset_end_save = m_offset_end.loadAcquire();
    offset_end += ((offset_end<offset_beg)?m_data_size:0);
  }
  if(offset_end - offset_beg < size) return false;
  memmove(reinterpret_cast<void*>(data), reinterpret_cast<void*>(m_data + offset_beg), size);
  offset_beg += size;
  offset_beg -= (offset_beg>m_data_size)?m_data_size:0;
  m_offset_beg.storeRelease(offset_beg);
  return true;
}

bool VMRingBuffer::write(char *data, qint64 size){
  qint64 offset_beg = m_offset_beg_save;
  qint64 offset_end = m_offset_end.loadAcquire();
  offset_beg += ((offset_beg<offset_end)?m_data_size:0);
  if(offset_beg - offset_end < size){
    offset_beg = m_offset_beg_save = m_offset_beg.loadAcquire();
    offset_beg += ((offset_beg<offset_end)?m_data_size:0);
  }
  if(offset_beg - offset_end < size) return false;
  memmove(reinterpret_cast<void*>(m_data+offset_end),reinterpret_cast<void*>(data),size);
  offset_end += size;
  offset_end -= ((offset_end>m_data_size)?m_data_size:0);
  m_offset_end.storeRelease(offset_end);
  return true;
}
