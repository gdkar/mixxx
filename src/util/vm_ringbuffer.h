#ifndef UTIL_VM_RINGBUFFER_H
#define UTIL_VM_RINGBUFFER_H

#include <qmath.h>
#include <qatomic.h>
#include <qsharedpointer.h>
#include <qthread.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#include "util/assert.h"

class VMRingBuffer {
  static quint64 s_page_size;
  char          *m_data;
  qint64         m_data_size;
  QAtomicInteger<qint64> m_offset_beg __attribute__((aligned(64)));
  qint64                 m_offset_end_save;
  QAtomicInteger<qint64> m_offset_end __attribute__((aligned(64)));
  qint64                 m_offset_beg_save;
  
public:
  explicit VMRingBuffer(qint64  size);
  virtual ~VMRingBuffer();
  virtual qint64  read_space();
  virtual qint64  write_space();
  virtual bool    write(char *data, qint64 size);
  virtual bool    read (char *data, qint64 size);
};
quint64 VMRingBuffer::s_page_size = 0;
#endif
