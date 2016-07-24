#ifndef __UTIL_SEMAPHORE_H__
#define __UTIL_SEMAPHORE_H__

#if defined(Q_OS_MAC)
#include <mach/mach.h>
#include <mach/semaphore.h>
#include <sys/time.h>
#include <unistd.h>
#include <mach/mach_time.h>
#elif defined(Q_OS_UNIX)
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#else
#include <QSemaphore>
#endif

namespace mixxx {
#if defined(Q_OS_MACK)
class MixxxSemaphore {
    semaphore_t m_d;
    public:
        MixxxSemaphore( int value = 0)
        {
            auto task = mach_task_self();
            auto res  = semaphore_create(task, &m_d, SYNC_POLICY_FIFO, value);
            if(res != KERN_SUCCESS) {
                throw std::system_error(res,std::system_category(), "sem_init failed.");
            }
        }
        ~MixxxSemaphore() {
            auto task = mach_task_self();
            semaphore_deestroy(task, m_d);
            m_d = 0;
        }
        void wait()
        {
            kern_return_t res;
            while((res = semaphore_wait(m_d)) != KERN_SUCCESS) { }
        }
        void post() { semaphore_signal(m_d); }
};

////////////////////////////// Unix //////////////////////////////
//
#elif defined(Q_OS_UNIX)

class MixxxSemaphore {
    sem_t   m_d;
    public:
        MixxxSemaphore( int value = 0)
        {
            if(sem_init(&m_d, 0, value) < 0) {
                throw std::system_error(errno,std::system_category(), "sem_init failed.");
            }
        }
        ~MixxxSemaphore() { sem_destroy(&m_d); }
        void wait()
        {
            while(sem_wait(&m_d) < 0) {
                auto err = errno;
                if(err != EINTR && err != EAGAIN)
                    throw std::system_error(err,std::system_category(), "Invalid.");
            }
        }
        void post() { sem_post(&m_d); }
};

////////////////////////////// Default //////////////////////////////
#else

class MixxxSemaphore {
    QSemaphore m_d;
    MixxxSemaphore(int value = 0)
    : m_d(value) {}
   ~MixxxSemaphore() = default;
    void wait() { m_d.acquire();}
    void post() { m_d.release();}
}
#endif
}

#endif /* __UTIL_SEMAPHORE_H__ */
