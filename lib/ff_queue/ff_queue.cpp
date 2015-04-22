/*  FastForward queue remix
 *  Copyright (c) 2011, Dmitry Vyukov
 *  Distributed under the terms of the GNU General Public License as published by the Free Software Foundation,
 *  either version 3 of the License, or (at your option) any later version.
 *  See: http://www.gnu.org/licenses
 */ 

// under linux build with -fno-strict-aliasing option

#include "stdafx.h"
#include "ff/ubuffer.hpp"
#include "ff/allocator.hpp"

#ifdef _MSC_VER
#   pragma warning (disable: 4200) // nonstandard extension used : zero-sized array in struct/union
#   define abort() __debugbreak(), (abort)()
#endif

unsigned const primes[16] = {1, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53};

struct random_generator
{
    unsigned k;
    unsigned c;
    unsigned x;

    void seed(uint64_t s)
    {
        k = ((unsigned)(s >> 32) & 0xf) + 8;
        c = primes[((unsigned)(s >> 36) & 0xf)];
        x = (unsigned)((s + 1) * 0x95949347 + c);
    }

    unsigned get()
    {
        return ((x = x + c + (x << k)) >> 16);
    }
};




class ff_queue
{
public:
    ff_queue (size_t buffer_size, size_t max_buffer_count)
        : buffer_size_ (buffer_size)
        , max_buffer_count_ (max_buffer_count)
    {
        buffer_count_ = 0;
        buffer_t* buffer = alloc_buffer(buffer_size_);
        head_pos_ = buffer->data;
        tail_pos_ = buffer->data;
        tail_end_ = buffer->data + buffer_size_;
        tail_next_ = 0;
        tail_buffer_ = buffer;
        last_buffer_ = buffer;
        *(void**)head_pos_ = (void*)eof;
        pad_[0] = 0;
    }

    ~ff_queue ()
    {
        buffer_t* buffer = last_buffer_;
        while (buffer != 0)
        {
            buffer_t* next_buffer = buffer->next;
            aligned_free(buffer);
            buffer = next_buffer;
        }
    }

    void* enqueue_prepare (size_t size)
    {
        // round-up message size for proper alignment
        size_t msg_size = ((uintptr_t)(size + sizeof(void*) - 1) & ~(sizeof(void*) - 1)) + sizeof(void*);
        // check as to whether there is enough space in the current buffer or not
        if ((size_t)(tail_end_ - tail_pos_) >= msg_size + sizeof(void*))
        {
            // if yes, remember where next message starts
            tail_next_ = tail_pos_ + msg_size;
            // and return a pointer into the buffer
            // for a user to fill with his data
            return tail_pos_ + sizeof(void*);
        }
        else
        {
            // otherwise, fall to slow-path
            return enqueue_prepare_slow(size);
        }
    }

    void enqueue_commit ()
    {
        // prepare next cell
        char* tail_next = tail_next_;
        *(char* volatile*)tail_next = (char*)eof;
        // update current cell
        // (after this point the message becomes consumable)
        atomic_addr_store_release((void* volatile*)tail_pos_, tail_next);
        tail_pos_ = tail_next;
    }

    void* dequeue_prepare ()
    {
        // load value in the current cell
        void* next = atomic_addr_load_acquire((void* volatile*)head_pos_);
        // if EOF flag is not set,
        // then there is a consumable message
        if (((uintptr_t)next & eof) == 0)
        {
            char* msg = head_pos_ + sizeof(void*);
            return msg;
        }
        // otherwise there is just nothing or ...
        else if (((uintptr_t)next & ~eof) == 0)
        {
            return 0;
        }
        // ... a tranfer to a next buffer
        else
        {
            // in this case we just follow the pointer and retry
            atomic_addr_store_release((void* volatile*)&head_pos_,
                (char*)((uintptr_t)next & ~eof));
            return dequeue_prepare();
        }
    }

    void dequeue_commit ()
    {
        // follow the pointer to the next cell
        char* next = *(char* volatile*)head_pos_;
        assert(next != 0);
        atomic_addr_store_release((void* volatile*)&head_pos_, next);
    }

private:
    struct buffer_t
    {
        // pointer to the next buffer in the queue
        buffer_t*               next;
        // size of the data
        size_t                  size;
        // the data
        char                    data [0];
    };

    // consumer part:
    // current position for reading
    // (points somewhere into buffer_t::data)
    char* volatile              head_pos_;

    // padding between consumer's and producer's parts
    char                        pad_ [CACHE_LINE_SIZE];

    // producer part:
    // current position for writing
    // (points somewhere into buffer_t::data)
    char*                       tail_pos_;
    // end of current buffer
    char*                       tail_end_;
    // helper variable
    char*                       tail_next_;
    // current 'tail' buffer
    buffer_t*                   tail_buffer_;
    // buffer cache
    buffer_t*                   last_buffer_;
    // default buffer size
    size_t const                buffer_size_;
    // desired number of cached buffers
    size_t const                max_buffer_count_;
    // current number of cached buffers
    size_t                      buffer_count_;

    // used as 'empty' marker
    static size_t const         eof = 1;

    buffer_t* alloc_buffer (size_t sz)
    {
        buffer_t* buffer = (buffer_t*)aligned_malloc(sizeof(buffer_t) + sz);
        if (buffer == 0)
            throw std::bad_alloc();
        buffer->next = 0;
        buffer->size = sz;
        buffer_count_ += 1;
        return buffer;
    }

    buffer_t* reuse_or_alloc_buffer (size_t& sz)
    {
        size_t buffer_size = buffer_size_;
        if (buffer_size < sz + 2 * sizeof(void*))
            buffer_size = sz + 2 * sizeof(void*);
        sz = buffer_size;

        buffer_t* buffer = 0;
        char* head_pos = (char*)atomic_addr_load_acquire((void* volatile*)&head_pos_);
        while (head_pos < last_buffer_->data || head_pos >= last_buffer_->data + last_buffer_->size)
        {
            buffer = last_buffer_;
            last_buffer_ = buffer->next;
            buffer->next = 0;
            assert(last_buffer_ != 0);

            if ((buffer->size < buffer_size)
                || (buffer_count_ > max_buffer_count_
                    && (head_pos < last_buffer_->data || head_pos >= last_buffer_->data + last_buffer_->size)))
            {
                aligned_free(buffer);
                buffer = 0;
                continue;
            }
            break;
        }

        if (buffer == 0)
            buffer = alloc_buffer(buffer_size);

        return buffer;
    }

    NOINLINE
    void* enqueue_prepare_slow (size_t sz)
    {
        size_t buffer_size = sz;
        buffer_t* buffer = reuse_or_alloc_buffer(buffer_size);
        *(void* volatile*)buffer->data = (void*)eof;
        atomic_addr_store_release((void* volatile*)tail_pos_, (void*)((uintptr_t)buffer->data | eof));
        tail_pos_ = buffer->data;
        tail_end_ = tail_pos_ + buffer_size;
        tail_buffer_->next = buffer;
        tail_buffer_ = buffer;
        return enqueue_prepare(sz);
    }

    ff_queue (ff_queue const&);
    void operator = (ff_queue const&);
};



class ff_queue2
{
public:
    ff_queue2 (size_t buffer_size, size_t max_buffer_count)
        : buffer_size_ (buffer_size)
        , max_buffer_count_ (max_buffer_count)
    {
        buffer_count_ = 0;
        buffer_t* buffer = alloc_buffer();
        head_pos_ = buffer->data;
        tail_pos_ = buffer->data;
        tail_end_ = buffer->data + buffer_size_;
        tail_buffer_ = buffer;
        last_buffer_ = buffer;
        head_pos_[0] = (void*)1;
        pad_[0] = 0;
    }

    ~ff_queue2 ()
    {
        buffer_t* buffer = last_buffer_;
        for (;;)
        {
            buffer_t* next_buffer = (buffer_t*)((uintptr_t)buffer->data[buffer_size_] & ~1);
            aligned_free(buffer);
            if (buffer == tail_buffer_)
                break;
            buffer = next_buffer;
        }
    }

    void enqueue (void* msg)
    {
        if (tail_pos_ < tail_end_)
        {
            tail_pos_[1] = (void*)1;
            atomic_addr_store_release(tail_pos_, msg);
            tail_pos_ += 1;
        }
        else
        {
            return enqueue_slow(msg);
        }
    }

    void* dequeue ()
    {
        void* msg = atomic_addr_load_acquire(head_pos_);
        if (((uintptr_t)msg & 1) == 0)
        {
            atomic_addr_store_release((void* volatile*)&head_pos_, (void*)(head_pos_ + 1));
            return msg;
        }
        else if (((uintptr_t)msg & ~1) == 0)
        {
            return 0;
        }
        else
        {
            atomic_addr_store_release((void* volatile*)&head_pos_, (char*)((uintptr_t)msg & ~1));
            return dequeue();
        }
    }

private:
    struct buffer_t
    {
        void* volatile          data [1];
    };

    void* volatile* volatile    head_pos_;

    char                        pad_ [CACHE_LINE_SIZE];

    void* volatile*             tail_pos_;
    void* volatile*             tail_end_;
    buffer_t*                   tail_buffer_;
    buffer_t*                   last_buffer_;
    size_t const                buffer_size_;
    size_t const                max_buffer_count_;
    size_t                      buffer_count_;

    buffer_t* alloc_buffer ()
    {
        buffer_t* buffer = (buffer_t*)aligned_malloc(sizeof(buffer_t) + buffer_size_ * sizeof(void*));
        if (buffer == 0)
            throw std::bad_alloc();
        buffer_count_ += 1;
        return buffer;
    }

    NOINLINE
    void enqueue_slow (void* msg)
    {
        buffer_t* buffer = 0;
        void* head_pos = atomic_addr_load_acquire((void* volatile*)&head_pos_);
        while (head_pos < last_buffer_->data || head_pos > last_buffer_->data + buffer_size_)
        {
            buffer = last_buffer_;
            last_buffer_ = (buffer_t*)((uintptr_t)buffer->data[buffer_size_] & ~1);
            assert(last_buffer_ != 0 && last_buffer_ != (void*)1);

            if (buffer_count_ > max_buffer_count_ && (head_pos < last_buffer_->data || head_pos >= last_buffer_->data + buffer_size_))
            {
                aligned_free(buffer);
                buffer = 0;
                continue;
            }
            break;
        }

        if (buffer == 0)
            buffer = alloc_buffer();

        buffer->data[0] = (void*)1;
        atomic_addr_store_release(tail_pos_, (void*)((uintptr_t)buffer->data | 1));
        tail_pos_ = buffer->data;
        tail_end_ = tail_pos_ + buffer_size_;
        tail_buffer_ = buffer;
        return enqueue(msg);
    }

    ff_queue2 (ff_queue2 const&);
    void operator = (ff_queue2 const&);
};


#define USE_FF_ALLOC

#ifdef USE_FF_ALLOC
#define ff_alloc(sz) ff::FFAllocator::instance()->malloc(sz)
#define ff_free(p) ff::FFAllocator::instance()->free(p)
#else
#define ff_alloc(sz) malloc(sz)
#define ff_free(p) free(p)
#endif



#if 0
#   define PRINTF(...) printf(__VA_ARGS__)
#else
#   define PRINTF(...)
#endif


size_t const buffer_size = 32*1024;
size_t const max_buffer_count = 32;
size_t const batch_size = 256;
size_t const batch_count = 200000;
unsigned const min_msg_size = 1;
unsigned const max_msg_size = 16;


std::vector<ff_queue*> queues;
std::vector<ff_queue2*> queues2;

void* worker_thread(void* ctx)
{
    ff_queue* in = queues[(size_t)ctx];
    ff_queue* out = queues[((size_t)ctx+1) % queues.size()];

    if (ctx == 0)
    {
        random_generator rnd;
        rnd.seed(time(0));

        char producer_seq = 0;
        char consumer_seq = 0;

        for (size_t b = 0; b != batch_count; b += 1)
        {
            for (size_t m = 0; m != batch_size; m += 1)
            {
                unsigned count = (rnd.get() % (max_msg_size - min_msg_size)) + min_msg_size;
                char* msg = (char*)out->enqueue_prepare(count + sizeof(unsigned));
                *(unsigned*)msg = count;
                msg += sizeof(unsigned);
                for (unsigned x = 0; x != count; x += 1)
                    msg[x] = producer_seq++;
                PRINTF("master >>> %u\n", count);
                out->enqueue_commit();
            }

            while (char* msg = (char*)in->dequeue_prepare())
            {
                unsigned count = *(unsigned*)msg;
                if (count < min_msg_size || count > max_msg_size)
                    printf("inconsistent size\n"), abort();
                msg += sizeof(unsigned);
                for (unsigned x = 0; x != count; x += 1)
                {
                    if (consumer_seq++ != msg[x])
                        printf("inconsistent data\n"), abort();
                }
                PRINTF("master <<< %u\n", count);
                in->dequeue_commit();
            }
        }

        char* msg = (char*)out->enqueue_prepare(sizeof(unsigned));
        *(unsigned*)msg = 0;
        PRINTF("master >>> EOF\n");
        out->enqueue_commit();

        for (;;)
        {
            char* msg = (char*)in->dequeue_prepare();
            if (msg == 0)
            {
                thread_yield();
                continue;
            }
            unsigned count = *(unsigned*)msg;
            if (count == 0)
            {
                PRINTF("master <<< EOF\n");
                break;
            }
            if (count < min_msg_size || count > max_msg_size)
                printf("inconsistent size\n"), abort();
            msg += sizeof(unsigned);
            for (unsigned x = 0; x != count; x += 1)
            {
                if (consumer_seq++ != msg[x])
                    printf("inconsistent data\n"), abort();
            }
            PRINTF("master <<< %u\n", count);
            in->dequeue_commit();
        }

        if (consumer_seq != producer_seq)
            printf("inconsistent seq\n"), abort();
    }
    else
    {
        char seq = 0;

        for (;;)
        {
            char* in_msg = (char*)in->dequeue_prepare();
            if (in_msg == 0)
            {
                thread_yield();
                continue;
            }
            unsigned count = *(unsigned*)in_msg;
            if ((count < min_msg_size || count > max_msg_size) && count != 0)
                printf("inconsistent size\n"), abort();
            in_msg += sizeof(unsigned);
            for (unsigned x = 0; x != count; x += 1)
            {
                if (seq++ != in_msg[x])
                    printf("inconsistent data\n"), abort();
            }

            char* out_msg = (char*)out->enqueue_prepare(count + sizeof(unsigned));
            *(unsigned*)out_msg = count;
            out_msg += sizeof(unsigned);
            for (size_t x = 0; x != count; x += 1)
                out_msg[x] = in_msg[x];

            if (count != 0)
                PRINTF("worker #%u <-> %u\n", (unsigned)(uintptr_t)ctx, count);
            else
                PRINTF("worker #%u <-> EOF\n", (unsigned)(uintptr_t)ctx);
            out->enqueue_commit();
            in->dequeue_commit();

            if (count == 0)
                break;
        }
    }

    return 0;
}

void* worker_thread2(void* ctx)
{
    ff_queue2* in = queues2[(size_t)ctx];
    ff_queue2* out = queues2[((size_t)ctx+1) % queues2.size()];

    if (ctx == 0)
    {
        random_generator rnd;
        rnd.seed(time(0));

        char producer_seq = 0;
        char consumer_seq = 0;

        for (size_t b = 0; b != batch_count; b += 1)
        {
            for (size_t m = 0; m != batch_size; m += 1)
            {
                unsigned count = (rnd.get() % (max_msg_size - min_msg_size)) + min_msg_size;
                char* msg = (char*)ff_alloc(count + sizeof(unsigned));
                *(unsigned*)msg = count;
                for (unsigned x = 0; x != count; x += 1)
                    msg[sizeof(unsigned) + x] = producer_seq++;
                PRINTF("master >>> %u\n", count);
                out->enqueue(msg);
            }

            while (char* msg = (char*)in->dequeue())
            {
                unsigned count = *(unsigned*)msg;
                if (count < min_msg_size || count > max_msg_size)
                    printf("inconsistent size\n"), abort();
                for (unsigned x = 0; x != count; x += 1)
                {
                    if (consumer_seq++ != msg[sizeof(unsigned) + x])
                        printf("inconsistent data\n"), abort();
                }
                PRINTF("master <<< %u\n", count);
                ff_free(msg);
            }
        }

        char* msg = (char*)ff_alloc(sizeof(unsigned));
        *(unsigned*)msg = 0;
        PRINTF("master >>> EOF\n");
        out->enqueue(msg);

        for (;;)
        {
            char* msg = (char*)in->dequeue();
            if (msg == 0)
            {
                thread_yield();
                continue;
            }
            unsigned count = *(unsigned*)msg;
            if (count == 0)
            {
                PRINTF("master <<< EOF\n");
                break;
            }
            if (count < min_msg_size || count > max_msg_size)
                printf("inconsistent size\n"), abort();
            for (unsigned x = 0; x != count; x += 1)
            {
                if (consumer_seq++ != msg[sizeof(unsigned) + x])
                    printf("inconsistent data\n"), abort();
            }
            PRINTF("master <<< %u\n", count);
            ff_free(msg);
        }

        if (consumer_seq != producer_seq)
            printf("inconsistent seq\n"), abort();
    }
    else
    {
        char seq = 0;

        for (;;)
        {
            char* in_msg = (char*)in->dequeue();
            if (in_msg == 0)
            {
                thread_yield();
                continue;
            }
            unsigned count = *(unsigned*)in_msg;
            if ((count < min_msg_size || count > max_msg_size) && count != 0)
                printf("inconsistent size\n"), abort();
            for (unsigned x = 0; x != count; x += 1)
            {
                if (seq++ != in_msg[sizeof(unsigned) + x])
                    printf("inconsistent data\n"), abort();
            }

            if (count != 0)
                PRINTF("worker #%u <-> %u\n", (unsigned)(uintptr_t)ctx, count);
            else
                PRINTF("worker #%u <-> EOF\n", (unsigned)(uintptr_t)ctx);

            out->enqueue(in_msg);

            if (count == 0)
                break;
        }
    }

    return 0;
}




std::vector<ff::uSWSR_Ptr_Buffer*> ff_queues;

void* ff_worker_thread(void* ctx)
{
    ff::uSWSR_Ptr_Buffer* in = ff_queues[(size_t)ctx];
    ff::uSWSR_Ptr_Buffer* out = ff_queues[((size_t)ctx+1) % ff_queues.size()];

    if (ctx == 0)
    {
        random_generator rnd;
        rnd.seed(time(0));

        char producer_seq = 0;
        char consumer_seq = 0;

        for (size_t b = 0; b != batch_count; b += 1)
        {
            for (size_t m = 0; m != batch_size; m += 1)
            {
                unsigned count = (rnd.get() % (max_msg_size - min_msg_size)) + min_msg_size;
                char* msg = (char*)ff_alloc(count + sizeof(unsigned));
                *(unsigned*)msg = count;
                for (unsigned x = 0; x != count; x += 1)
                    msg[sizeof(unsigned) + x] = producer_seq++;
                PRINTF("master >>> %u\n", count);
                out->push(msg);
            }

            char* msg;
            while (in->pop((void**)&msg))
            {
                unsigned count = *(unsigned*)msg;
                if (count < min_msg_size || count > max_msg_size)
                    printf("inconsistent size\n"), abort();
                for (unsigned x = 0; x != count; x += 1)
                {
                    if (consumer_seq++ != msg[sizeof(unsigned) + x])
                        printf("inconsistent data\n"), abort();
                }
                PRINTF("master <<< %u\n", count);
                ff_free(msg);
            }
        }

        char* msg = (char*)ff_alloc(sizeof(unsigned));
        *(unsigned*)msg = 0;
        PRINTF("master >>> EOF\n");
        out->push(msg);

        for (;;)
        {
            char* msg;
            if (!in->pop((void**)&msg))
            {
                thread_yield();
                continue;
            }
            unsigned count = *(unsigned*)msg;
            if (count == 0)
            {
                PRINTF("master <<< EOF\n");
                break;
            }
            if (count < min_msg_size || count > max_msg_size)
                printf("inconsistent size\n"), abort();
            for (unsigned x = 0; x != count; x += 1)
            {
                if (consumer_seq++ != msg[sizeof(unsigned) + x])
                    printf("inconsistent data\n"), abort();
            }
            PRINTF("master <<< %u\n", count);
            ff_free(msg);
        }

        if (consumer_seq != producer_seq)
            printf("inconsistent seq\n"), abort();
    }
    else
    {
        char seq = 0;

        for (;;)
        {
            char* in_msg;
            if (!in->pop((void**)&in_msg))
            {
                thread_yield();
                continue;
            }
            unsigned count = *(unsigned*)in_msg;
            if ((count < min_msg_size || count > max_msg_size) && count != 0)
                printf("inconsistent size\n"), abort();
            for (unsigned x = 0; x != count; x += 1)
            {
                if (seq++ != in_msg[sizeof(unsigned) + x])
                    printf("inconsistent data\n"), abort();
            }

            char* out_msg = (char*)ff_alloc(count + sizeof(unsigned));
            for (size_t x = 0; x != sizeof(unsigned) + count; x += 1)
                out_msg[x] = in_msg[x];
            ff_free(in_msg);

            if (count != 0)
                PRINTF("worker #%u <-> %u\n", (unsigned)(uintptr_t)ctx, count);
            else
                PRINTF("worker #%u <-> EOF\n", (unsigned)(uintptr_t)ctx);

            out->push(out_msg);

            if (count == 0)
                break;
        }
    }

    return 0;
}



void* ff_worker_thread2(void* ctx)
{
    ff::uSWSR_Ptr_Buffer* in = ff_queues[(size_t)ctx];
    ff::uSWSR_Ptr_Buffer* out = ff_queues[((size_t)ctx+1) % ff_queues.size()];

    if (ctx == 0)
    {
        PRINTF("main worker: started\n");

        random_generator rnd;
        rnd.seed(time(0));

        char producer_seq = 0;
        char consumer_seq = 0;

        for (size_t b = 0; b != batch_count; b += 1)
        {
            for (size_t m = 0; m != batch_size; m += 1)
            {
                unsigned count = (rnd.get() % (max_msg_size - min_msg_size)) + min_msg_size;
                char* msg = (char*)ff_alloc(count + sizeof(unsigned));
                *(unsigned*)msg = count;
                for (unsigned x = 0; x != count; x += 1)
                    msg[sizeof(unsigned) + x] = producer_seq++;
                PRINTF("master >>> %u\n", count);
                out->push(msg);
            }

            char* msg;
            while (in->pop((void**)&msg))
            {
                unsigned count = *(unsigned*)msg;
                if (count < min_msg_size || count > max_msg_size)
                    printf("inconsistent size\n"), abort();
                for (unsigned x = 0; x != count; x += 1)
                {
                    if (consumer_seq++ != msg[sizeof(unsigned) + x])
                        printf("inconsistent data\n"), abort();
                }
                PRINTF("master <<< %u\n", count);
                ff_free(msg);
            }
        }

        char* msg = (char*)ff_alloc(sizeof(unsigned));
        *(unsigned*)msg = 0;
        PRINTF("master >>> EOF\n");
        out->push(msg);

        for (;;)
        {
            char* msg;
            if (!in->pop((void**)&msg))
            {
                thread_yield();
                continue;
            }
            unsigned count = *(unsigned*)msg;
            if (count == 0)
            {
                PRINTF("master <<< EOF\n");
                ff_free(msg);
                break;
            }
            if (count < min_msg_size || count > max_msg_size)
                printf("inconsistent size\n"), abort();
            for (unsigned x = 0; x != count; x += 1)
            {
                if (consumer_seq++ != msg[sizeof(unsigned) + x])
                    printf("inconsistent data\n"), abort();
            }
            PRINTF("master <<< %u\n", count);
            ff_free(msg);
        }

        if (consumer_seq != producer_seq)
            printf("inconsistent seq\n"), abort();
    }
    else
    {
        PRINTF("worker #u: started\n", (unsigned)(uintptr_t)ctx);

        char seq = 0;

        for (;;)
        {
            char* in_msg;
            if (!in->pop((void**)&in_msg))
            {
                thread_yield();
                continue;
            }
            unsigned count = *(unsigned*)in_msg;
            if ((count < min_msg_size || count > max_msg_size) && count != 0)
                printf("inconsistent size\n"), abort();
            for (unsigned x = 0; x != count; x += 1)
            {
                if (seq++ != in_msg[sizeof(unsigned) + x])
                    printf("inconsistent data\n"), abort();
            }

            if (count != 0)
                PRINTF("worker #%u <-> %u\n", (unsigned)(uintptr_t)ctx, count);
            else
                PRINTF("worker #%u <-> EOF\n", (unsigned)(uintptr_t)ctx);

            out->push(in_msg);

            if (count == 0)
                break;
        }
    }

    return 0;
}



int main(int argc, char** argv)
{
    if (argc == 1)
        printf("usage: ffq ff/ff2/my/my2/all\n"), exit(1);

#ifdef USE_FF_ALLOC
    ff::FFAllocator::instance();
#endif

    size_t proc_count = get_proc_count();
    if (argc > 2 && atoi(argv[2]) > 0)
        proc_count = atoi(argv[2]);

    long long time = 0;

    std::vector<thread_t> threads (proc_count);

    if (strcmp(argv[1], "ff") == 0)
    {
        ff_queues.resize (proc_count);
        for (size_t i = 0; i != proc_count; i += 1)
        {
            ff_queues[i] = new (aligned_malloc(sizeof(ff::uSWSR_Ptr_Buffer))) ff::uSWSR_Ptr_Buffer (buffer_size / 16);
            ff_queues[i]->init();
        }

        time = get_nano_time(1);

        for (size_t i = 0; i != proc_count; i += 1)
            thread_start(&threads[i], ff_worker_thread, (void*)i);
    }
    else if (strcmp(argv[1], "ff2") == 0)
    {
        ff_queues.resize (proc_count);
        for (size_t i = 0; i != proc_count; i += 1)
        {
            ff_queues[i] = new (aligned_malloc(sizeof(ff::uSWSR_Ptr_Buffer))) ff::uSWSR_Ptr_Buffer (buffer_size / 16);
            ff_queues[i]->init();
        }

        time = get_nano_time(1);

        PRINTF("main: waiting for threads\n");

        for (size_t i = 0; i != proc_count; i += 1)
            thread_start(&threads[i], ff_worker_thread2, (void*)i);
    }
    else if (strcmp(argv[1], "my") == 0)
    {
        queues.resize (proc_count);
        for (size_t i = 0; i != proc_count; i += 1)
            queues[i] = new (aligned_malloc(sizeof(ff_queue))) ff_queue (buffer_size, max_buffer_count);

        time = get_nano_time(1);

        for (size_t i = 0; i != proc_count; i += 1)
            thread_start(&threads[i], worker_thread, (void*)i);
    }
    else if (strcmp(argv[1], "my2") == 0)
    {
        queues2.resize (proc_count);
        for (size_t i = 0; i != proc_count; i += 1)
            queues2[i] = new (aligned_malloc(sizeof(ff_queue))) ff_queue2 (buffer_size / 16, max_buffer_count);

        time = get_nano_time(1);

        for (size_t i = 0; i != proc_count; i += 1)
            thread_start(&threads[i], worker_thread2, (void*)i);
    }
    else
    {
        printf("my_copy");
        for (size_t th = 1; th <= proc_count; th += 1)
        {
            queues.resize (th);
            for (size_t i = 0; i != th; i += 1)
                queues[i] = new (aligned_malloc(sizeof(ff_queue))) ff_queue (buffer_size, max_buffer_count);
            time = get_nano_time(1);
            for (size_t i = 0; i != th; i += 1)
                thread_start(&threads[i], worker_thread, (void*)i);
            for (size_t i = 0; i != th; i += 1)
                thread_join(&threads[i]);
            time = get_nano_time(0) - time;
            unsigned throughput = (unsigned)(1000000ull * batch_size * batch_count * th / time);
            printf(" %u", throughput);
            fflush(stdout);
            for (size_t i = 0; i != th; i += 1)
            {
                queues[i]->~ff_queue();
                aligned_free(queues[i]);
            }

            ff::FFAllocator::instance()->~FFAllocator();
            new (ff::FFAllocator::instance()) ff::FFAllocator;
        }
        printf("\n");

        printf("my_ptr");
        for (size_t th = 1; th <= proc_count; th += 1)
        {
            queues2.resize (th);
            for (size_t i = 0; i != th; i += 1)
                queues2[i] = new (aligned_malloc(sizeof(ff_queue2))) ff_queue2 (buffer_size / 16, max_buffer_count);
            time = get_nano_time(1);
            for (size_t i = 0; i != th; i += 1)
                thread_start(&threads[i], worker_thread2, (void*)i);
            for (size_t i = 0; i != th; i += 1)
                thread_join(&threads[i]);
            time = get_nano_time(0) - time;
            unsigned throughput = (unsigned)(1000000ull * batch_size * batch_count * th / time);
            printf(" %u", throughput);
            fflush(stdout);
            for (size_t i = 0; i != th; i += 1)
            {
                queues2[i]->~ff_queue2();
                aligned_free(queues2[i]);
            }

            ff::FFAllocator::instance()->~FFAllocator();
            new (ff::FFAllocator::instance()) ff::FFAllocator;

        }
        printf("\n");

        printf("ff_copy");
        for (size_t th = 1; th <= proc_count; th += 1)
        {
            ff_queues.resize (th);
            for (size_t i = 0; i != th; i += 1)
            {
                ff_queues[i] = new (aligned_malloc(sizeof(ff::uSWSR_Ptr_Buffer))) ff::uSWSR_Ptr_Buffer (buffer_size / 16);
                ff_queues[i]->init();
            }
            time = get_nano_time(1);
            for (size_t i = 0; i != th; i += 1)
                thread_start(&threads[i], ff_worker_thread, (void*)i);
            for (size_t i = 0; i != th; i += 1)
                thread_join(&threads[i]);
            time = get_nano_time(0) - time;
            unsigned throughput = (unsigned)(1000000ull * batch_size * batch_count * th / time);
            printf(" %u", throughput);
            fflush(stdout);
            for (size_t i = 0; i != th; i += 1)
            {
                ff_queues[i]->~uSWSR_Ptr_Buffer();
                aligned_free(ff_queues[i]);
            }

            ff::FFAllocator::instance()->~FFAllocator();
            new (ff::FFAllocator::instance()) ff::FFAllocator;

        }
        printf("\n");

        printf("ff_ptr");
        for (size_t th = 1; th <= proc_count; th += 1)
        {
            ff_queues.resize (th);
            for (size_t i = 0; i != th; i += 1)
            {
                ff_queues[i] = new (aligned_malloc(sizeof(ff::uSWSR_Ptr_Buffer))) ff::uSWSR_Ptr_Buffer (buffer_size / 16);
                ff_queues[i]->init();
            }
            time = get_nano_time(1);
            for (size_t i = 0; i != th; i += 1)
                thread_start(&threads[i], ff_worker_thread2, (void*)i);
            for (size_t i = 0; i != th; i += 1)
                thread_join(&threads[i]);
            time = get_nano_time(0) - time;
            unsigned throughput = (unsigned)(1000000ull * batch_size * batch_count * th / time);
            printf(" %u", throughput);
            fflush(stdout);
            for (size_t i = 0; i != th; i += 1)
            {
                ff_queues[i]->~uSWSR_Ptr_Buffer();
                aligned_free(ff_queues[i]);
            }

            ff::FFAllocator::instance()->~FFAllocator();
            new (ff::FFAllocator::instance()) ff::FFAllocator;

        }
        printf("\n");

        return 0;
    }

    for (size_t i = 0; i != proc_count; i += 1)
        thread_join(&threads[i]);

    time = get_nano_time(0) - time;
    unsigned throughput = (unsigned)(1000000ull * batch_size * batch_count * proc_count / time);

    printf("exec_time = %u ms, throughput = %u msg/ms\n", (unsigned)(time / 1000000), throughput);
}

