#ifndef CONTROLVALUE_H
#define CONTROLVALUE_H

#include <limits>

#include <atomic>
#include <QObject>

#include "util/compatibility.h"
#include "util/assert.h"



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
    // there are basicly unlimited readers allowed at each ring element
    // but we have to count them so max() is just fine.
    // NOTE(rryan): Wrapping max with parentheses avoids conflict with the max macro
    // defined in windows.h.
    static constexpr const int cReaderSlotCnt = (std::numeric_limits<int>::max)();
    ControlRingValue()
        : m_value(),
          m_readerSlots(cReaderSlotCnt)
    { }

    bool tryGet(T* value) const
    {
        // Read while consuming one readerSlot
        auto hasSlot = m_readerSlots.fetch_add(-1) > 0;
        if (hasSlot) {
            *value = m_value;
        }
        (void)m_readerSlots.fetch_add(1);
        return hasSlot;
    }

    bool trySet(const T& value)
    {
        // try to lock this element entirely for reading
        auto expected = m_readerSlots.load(std::memory_order_acquire);
        if(expected == cReaderSlotCnt
        && m_readerSlots.compare_exchange_strong(expected, 0)) {
            m_value = value;
            m_readerSlots.fetch_add(cReaderSlotCnt);
            return true;
        }
        return false;
   }
  private:
    T m_value{};
    mutable std::atomic<int> m_readerSlots{cReaderSlotCnt};
};

// Ring buffer based implementation for all Types sizeof(T) > sizeof(void*)

// An implementation of ControlValueAtomicBase for non-atomic types T. Uses a
// ring-buffer of ControlRingValues and a read pointer and write pointer to
// provide getValue()/setValue() methods which *sacrifice perfect consistency*
// for the benefit of wait-free read/write access to a value.
template<typename T, bool ATOMIC = false>
class ControlValueAtomicBase {
  public:
    // for lock free access, this value has to be >= the number of value using threads
    // value must be a fraction of an integer
    static constexpr const int cRingSize = 8;
    static_assert(((std::numeric_limits<unsigned int>::max)() % cRingSize) == (cRingSize - 1)
        , "cRingSize must divide UINT_MAX + 1.");
    T getValue() const
    {
        auto value = T{};
        auto index = uint32_t(m_readIndex.load());
        while (!m_ring[index%cRingSize].tryGet(&value)) {
            // We are here if
            // 1) there are more then cReaderSlotCnt reader (get) reading the same value or
            // 2) the formerly current value is locked by a writer
            // Case 1 does not happen because we have enough (0x7fffffff) reader slots.
            // Case 2 happens when the a reader is delayed after reading the
            // m_currentIndex and in the mean while a reader locks the formaly current value
            // because it has written cRingSize times. Reading the less recent value will fix
            // it because it is now actualy the current value.
            index--;
        }
        return value;
    }
    void setValue(const T& value)
    {
        // Test if we can read atomic
        // This test is const and will be mad only at compile time
        auto index = uint32_t{};
        do {
            index = m_writeIndex.fetch_add(1,std::memory_order_acquire);
            // This will be repeated if the value is locked
            // 1) by another writer writing at the same time or
            // 2) a delayed reader is still blocking the formerly current value
            // In both cases writing to the next value will fix it.
        } while (!m_ring[index%cRingSize].trySet(value));
        m_readIndex = (int)index;
    }
  protected:
    ControlValueAtomicBase() = default;
  private:
    // In worst case, each reader can consume a reader slot from a different ring element.
    // In this case there is still one ring element available for writing.
    ControlRingValue<T> m_ring[cRingSize];
    std::atomic<int> m_readIndex{0};
    std::atomic<int> m_writeIndex{1};
};

// Specialized template for types that are deemed to be atomic on the target
// architecture. Instead of using a read/write ring to guarantee atomicity,
// direct assignment/read of an aligned member variable is used.
template<typename T>
class ControlValueAtomicBase<T, true> {
  public:
    T getValue() const
    {
        return m_value.load();
    }
    void setValue(const T& value)
    {
        m_value = value;
    }
  protected:
    ControlValueAtomicBase() = default;
  private:
    std::atomic<T> m_value{};
};

// ControlValueAtomic is a wrapper around ControlValueAtomicBase which uses the
// sizeof(T) to determine which underlying implementation of
// ControlValueAtomicBase to use. For types where sizeof(T) <= sizeof(void*),
// the specialized implementation of ControlValueAtomicBase for types that are
// atomic on the architecture is used.
template<typename T>
class ControlValueAtomic
    : public ControlValueAtomicBase<T, std::is_trivially_copyable<T>::value && sizeof(T) <= sizeof(long double)> {
  public:
    ControlValueAtomic() = default;
};

#endif /* CONTROLVALUE_H */
