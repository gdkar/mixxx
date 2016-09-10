#ifndef CONTROLVALUE_H
#define CONTROLVALUE_H

#include <limits>
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <atomic>
#include <QObject>

#include "util/assert.h"

// for lock free access, this value has to be >= the number of value using threads
// value must be a fraction of an integer
constexpr std::size_t cRingSize = 32;
// there are basicly unlimited readers allowed at each ring element
// but we have to count them so max() is just fine.
// NOTE(rryan): Wrapping max with parentheses avoids conflict with the max macro
// defined in windows.h.
constexpr std::size_t cReaderSlotCnt = (std::numeric_limits<int>::max)();

// A single instance of a value of type T along with an atomic integer which
// tracks the current number of readers or writers of the slot. The value
// m_readerSlots starts at cReaderSlotCnt and counts down to 0. If the value is
// 0 or less then reads to the value fail because there are either too many
// readers or a write is occurring. A write to the value will fail if
// m_readerSlots is not equal to cReaderSlotCnt (e.g. there is an active
// reader).
template<typename T>
class ControlRingValue {
  public:
    ControlRingValue(T init = T{}) : m_value(init) { }

    bool tryGet(T &value) const
    {
        auto hasSlot = m_readerSlots.fetch_add(-1,std::memory_order_acquire) > 0;
        if(hasSlot)
            value = m_value;
        m_readerSlots.fetch_add(1,std::memory_order_release);
        return hasSlot;
    }
    bool trySet(const T& value)
    {
        auto expected = cReaderSlotCnt;
        if(m_readerSlots.compare_exchange_strong(expected, 0, std::memory_order_acquire,std::memory_order_relaxed)) {
            m_value = value;
            m_readerSlots.fetch_add(cReaderSlotCnt,std::memory_order_release);
            return true;
        }
        return false;
   }
  private:
    T m_value{};
    mutable std::atomic<size_t> m_readerSlots{cReaderSlotCnt};
};

// Ring buffer based implementation for all Types sizeof(T) > sizeof(void*)

// An implementation of ControlValueAtomicBase for non-atomic types T. Uses a
// ring-buffer of ControlRingValues and a read pointer and write pointer to
// provide getValue()/setValue() methods which *sacrifice perfect consistency*
// for the benefit of wait-free read/write access to a value.
template<typename T>
struct ControlValueAtomicRing {
    ControlRingValue<T> m_ring[cRingSize];
    std::atomic<size_t> m_readIndex{0};
    std::atomic<size_t> m_writeIndex{1};
    T getValue() const
    {
        auto val = T{};
        auto ridx = m_readIndex.load(std::memory_order_acquire);
        while (!m_ring[ridx % cRingSize].tryGet(val)) {
          ++ridx;
        }
        return val;
    }
    void setValue(const T& value)
    {
        auto widx = std::size_t{};
        // Test if we can read atomic
        // This test is const and will be mad only at compile time
        do{
            widx = m_writeIndex.fetch_add(1,std::memory_order_acquire);
        }while(!m_ring[widx % cRingSize].trySet(value));
        auto ridx = m_readIndex.load(std::memory_order_relaxed);
        while( ridx < widx
           && !m_readIndex.compare_exchange_strong(
                   ridx
                 , widx
                 , std::memory_order_release
                 , std::memory_order_relaxed
                   ))
        {
            // wait
        }
    }
    void setValue(T&& value)
    {
        auto widx = std::size_t{};
        // Test if we can read atomic
        // This test is const and will be mad only at compile time
        do{
            widx = m_writeIndex.fetch_add(1,std::memory_order_acquire);
        }while(!m_ring[widx % cRingSize].trySet(std::forward<T>(value)));
        auto ridx = m_readIndex.load(std::memory_order_relaxed);
        while( ridx < widx
           && !m_readIndex.compare_exchange_strong(
                   ridx
                 , widx
                 , std::memory_order_release
                 , std::memory_order_relaxed
                   ))
        {
            // wait
        }
    }

  protected:
    ControlValueAtomicRing()
        : m_readIndex(0),
          m_writeIndex(1)
    {
        // NOTE(rryan): Wrapping max with parentheses avoids conflict with the
        // max macro defined in windows.h.
        static_assert(
            (std::numeric_limits<unsigned int>::max() % cRingSize) == (cRingSize - 1)
            ,"Something's gone all fuuunky. <_<");
    }
    // In worst case, each reader can consume a reader slot from a different ring element.
    // In this case there is still one ring element available for writing.
};

// Specialized template for types that are deemed to be atomic on the target
// architecture. Instead of using a read/write ring to guarantee atomicity,
// direct assignment/read of an aligned member variable is used.
template<typename T>
struct ControlValueAtomicNative{
    std::atomic<T> m_value{};
    T getValue() const
    { return m_value.load(); }
    void setValue(const T& val) { m_value.store(val); }
    void setValue(T && val) { m_value.store(std::forward<T>(val)); }
    T testAndSet(const T &val) { return m_value.exchange(val);}
    bool compareAndSwap(T &expected, const T&desired)
    {
        return m_value.compare_exchange_strong(expected, desired);
    }
protected:
    ControlValueAtomicNative(T init = T{}) : m_value{init} { }
};

// ControlValueAtomic is a wrapper around ControlValueAtomicBase which uses the
// sizeof(T) to determine which underlying implementation of
// ControlValueAtomicBase to use. For types where sizeof(T) <= sizeof(void*),
// the specialized implementation of ControlValueAtomicBase for types that are
// atomic on the architecture is used.
template<typename T>
class ControlValueAtomic : public std::conditional_t<
    (std::is_trivially_copyable<T>::value && (sizeof(T) < sizeof(long double)))
   , ControlValueAtomicNative<T>
   , ControlValueAtomicRing<T>
    > {
    using super = std::conditional_t<
            (std::is_trivially_copyable<T>::value && (sizeof(T) < sizeof(long double)))
          , ControlValueAtomicNative<T>
          , ControlValueAtomicRing<T>
            >;
  public:
    ControlValueAtomic() = default;
};

#endif /* CONTROLVALUE_H */
