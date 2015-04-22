/*  FastForward queue remix
 *  Copyright (c) 2011, Dmitry Vyukov
 *  Distributed under the terms of the GNU General Public License as published by the Free Software Foundation,
 *  either version 3 of the License, or (at your option) any later version.
 *  See: http://www.gnu.org/licenses
 */ 

#pragma once

#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include <stdint.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/syscall.h>


#define INLINE static __inline
#define NOINLINE
#define CACHE_LINE_SIZE 128


INLINE void* aligned_malloc(size_t sz)
{
    void*               mem;
    if (posix_memalign(&mem, CACHE_LINE_SIZE, sz))
        return 0;
    return mem;
}


INLINE void aligned_free(void* mem)
{
    free(mem);
}


INLINE int get_proc_count()
{
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
}


INLINE long long get_nano_time(int start)
{
    struct timespec     time;
    (void)start;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * 1000000000ull + time.tv_nsec;
}


INLINE unsigned long long rdtsc()
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}


INLINE void sleep_ms(int msec)
{
    usleep(msec * 1000);
}


INLINE void atomic_signal_fence()
{
    __asm __volatile ("" ::: "memory");
}


INLINE void atomic_thread_fence()
{
    __asm __volatile ("mfence" ::: "memory");
}


INLINE int atomic_add_fetch(int volatile* addr, int val)
{
    return __sync_add_and_fetch(addr, val);
}


INLINE void atomic_addr_store_release(void* volatile* addr, void* val)
{
    __asm __volatile ("" ::: "memory");
    addr[0] = val;
}


INLINE void* atomic_addr_load_acquire(void* volatile* addr)
{
    void* val;
    val = addr[0];
    __asm __volatile ("" ::: "memory");
    return val;
}

INLINE uint64_t atomic_uint64_compare_exchange(uint64_t volatile* addr, uint64_t cmp, uint64_t xchg)
{
    return __sync_val_compare_and_swap(addr, cmp, xchg);
}

INLINE void* atomic_addr_compare_exchange(void* volatile* addr, void* cmp, void* xchg)
{
    return __sync_val_compare_and_swap(addr, cmp, xchg);
}

INLINE unsigned atomic_uint_fetch_add(unsigned volatile* addr, unsigned val)
{
    return __sync_fetch_and_add(addr, val);
}

INLINE unsigned atomic_uint_exchange(unsigned volatile* addr, unsigned val)
{
    unsigned cmp = *addr;
    for (;;)
    {
        unsigned prev = __sync_val_compare_and_swap(addr, cmp, val);
        if (cmp == prev)
            return cmp;
        cmp = prev;
    }
}

INLINE void atomic_uint_store_release(unsigned volatile* addr, unsigned val)
{
    __asm __volatile ("" ::: "memory");
    *addr = val;
}

INLINE unsigned atomic_uint_load_acquire(unsigned volatile* addr)
{
    unsigned val = *addr;
    __asm __volatile ("" ::: "memory");
    return val;
}



INLINE void core_yield()
{
    __asm __volatile ("pause" ::: "memory");
}


INLINE void thread_yield()
{
    sched_yield();
}


typedef pthread_t thread_t;


INLINE void thread_start(thread_t* th, void*(*func)(void*), void* arg)
{
    pthread_create(th, 0, func, arg);
}


INLINE void thread_join(thread_t* th)
{
    void*                       tmp;
    pthread_join(th[0], &tmp);
}


INLINE void thread_setup_prio()
{
    //struct sched_param param;
    //param.sched_priority = 30;
    //if (pthread_setschedparam(pthread_self(), SCHED_OTHER, &param))
    //    printf("failed to set thread prio\n");
}


typedef sem_t semaphore_t;


INLINE void semaphore_create(semaphore_t* sem)
{
    sem_init(sem, 0, 0);
}


INLINE void semaphore_destroy(semaphore_t* sem)
{
    sem_destroy(sem);
}


INLINE void semaphore_post(semaphore_t* sem)
{
    sem_post(sem);
}


INLINE void semaphore_wait(semaphore_t* sem)
{
    sem_wait(sem);
}



typedef pthread_mutex_t mutex_t;

INLINE void mutex_create(mutex_t* mtx)
{
    pthread_mutex_init(mtx, 0);
}

INLINE void mutex_destroy(mutex_t* mtx)
{
    pthread_mutex_destroy(mtx);
}

INLINE void mutex_lock(mutex_t* mtx)
{
    pthread_mutex_lock(mtx);
}

INLINE void mutex_unlock(mutex_t* mtx)
{
    pthread_mutex_unlock(mtx);
}



typedef pthread_key_t tls_slot_t;

INLINE void tls_create(tls_slot_t* slot)
{
    pthread_key_create(slot, 0);
}

INLINE void tls_destroy(tls_slot_t* slot)
{
    pthread_key_delete(*slot);
}

INLINE void* tls_get(tls_slot_t* slot)
{
    return pthread_getspecific(*slot);
}

INLINE void tls_set(tls_slot_t* slot, void* value)
{
    pthread_setspecific(*slot, value);
}


