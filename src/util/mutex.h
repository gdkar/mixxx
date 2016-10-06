#ifndef UTIL_MUTEX_H
#define UTIL_MUTEX_H

// Thread annotation aware variants of locks, read-write locks and scoped
// lockers. This allows us to use Clang thread safety analysis in Mixxx.
// See: http://clang.llvm.org/docs/ThreadSafetyAnalysis.html

#include <QMutex>
#include <QReadWriteLock>
#include <QMutexLocker>

#include <mutex>
#include <thread>
#include <atomic>
#include <shared_mutex>
#include <condition_variable>

template<class T>
std::unique_lock<T> lock_uniquely(T &t) { return std::unique_lock<T>(t); }

#include "util/thread_annotations.h"

class CAPABILITY("mutex") MMutex {
  public:
    MMutex(QMutex::RecursionMode mode = QMutex::NonRecursive)
            : m_mutex(mode) {
    }

    void lock() ACQUIRE() { m_mutex.lock(); }
    void unlock() RELEASE() { m_mutex.unlock(); }
    bool tryLock() TRY_ACQUIRE(true) {
        return m_mutex.tryLock();
    }

  private:
    QMutex m_mutex;
    friend class MMutexLocker;
};

class CAPABILITY("mutex") MReadWriteLock {
  public:
    MReadWriteLock(QReadWriteLock::RecursionMode mode = QReadWriteLock::NonRecursive)
            : m_lock(mode) {
    }

    void lockForRead() ACQUIRE_SHARED() { m_lock.lockForRead(); }
    bool tryLockForRead() TRY_ACQUIRE_SHARED(true) {
        return m_lock.tryLockForRead();
    }

    void lockForWrite() ACQUIRE() { m_lock.lockForWrite(); }
    bool tryLockForWrite() TRY_ACQUIRE(true) {
        return m_lock.tryLockForWrite();
    }

    void unlock() RELEASE() { m_lock.unlock(); }

  private:
    QReadWriteLock m_lock;
    friend class MWriteLocker;
    friend class MReadLocker;
};

class SCOPED_CAPABILITY MMutexLocker {
  public:
    MMutexLocker(MMutex* mu) ACQUIRE(mu) : m_locker(&mu->m_mutex) {}
    ~MMutexLocker() RELEASE() {}

    void unlock() RELEASE() { m_locker.unlock(); }

  private:
    QMutexLocker m_locker;
};

class SCOPED_CAPABILITY MWriteLocker {
  public:
    MWriteLocker(MReadWriteLock* mu) ACQUIRE(mu) : m_locker(&mu->m_lock) {}
    ~MWriteLocker() RELEASE() {}

    void unlock() RELEASE() { m_locker.unlock(); }

  private:
    QWriteLocker m_locker;
};

class SCOPED_CAPABILITY MReadLocker {
  public:
    MReadLocker(MReadWriteLock* mu) ACQUIRE_SHARED(mu)
            : m_locker(&mu->m_lock) {}
    ~MReadLocker() RELEASE() {}

    void unlock() RELEASE() { m_locker.unlock(); }

  private:
    QReadLocker m_locker;
};

#endif /* UTIL_MUTEX_H */
