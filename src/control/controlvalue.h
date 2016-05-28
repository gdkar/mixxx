_Pragma("once")
#include <limits>
#include <atomic>

#include "util/compatibility.h"
#include "util/assert.h"

// for look free access, this value has to be >= the number of value using threads
// value must be a fraction of an integer
constexpr const size_t cRingSize = 8;
// there are basicly unlimited readers allowed at each ring element
// but we have to count them so max() is just fine.
// NOTE(rryan): Wrapping max with parentheses avoids conflict with the max macro
// defined in windows.h.
constexpr const size_t cReaderSlotCnt = (std::numeric_limits<int>::max)();

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
    ControlRingValue() = default;
    bool tryGet(T* value) const {
        // Read while consuming one readerSlot
        auto hasSlot = (m_readerSlots.fetch_add(-1,std::memory_order_acquire) > 0);
        if (hasSlot)
            *value = m_value;
        (void)m_readerSlots.fetch_add(1,std::memory_order_release);
        return hasSlot;
    }
    bool trySet(const T& value) {
        // try to lock this element entirely for reading
        auto expected = cReaderSlotCnt;
        if (m_readerSlots.compare_exchange_strong(expected, 0, std::memory_order_acquire)) {
            m_value = value;
            m_readerSlots.store(cReaderSlotCnt);
            return true;
        }
        return false;
   }
  private:
    T                            m_value{};
    mutable std::atomic<size_t > m_readerSlots{cReaderSlotCnt};
};
// Ring buffer based implementation for all Types sizeof(T) > sizeof(void*)

// An implementation of ControlValueAtomicBase for non-atomic types T. Uses a
// ring-buffer of ControlRingValues and a read pointer and write pointer to
// provide getValue()/setValue() methods which *sacrifice perfect consistency*
// for the benefit of wait-free read/write access to a value.
template<typename T, bool ATOMIC = false>
class ControlValueAtomicBase {
  public:
    inline T getValue() const
    {
        T value = T();
        auto index = m_readIndex.load();
        while (m_ring[index & (cRingSize-1)].tryGet(&value) == false) {
            // We are here if
            // 1) there are more then cReaderSlotCnt reader (get) reading the same value or
            // 2) the formerly current value is locked by a writer
            // Case 1 does not happen because we have enough (0x7fffffff) reader slots.
            // Case 2 happens when the a reader is delayed after reading the
            // m_currentIndex and in the mean while a reader locks the formaly current value
            // because it has written cRingSize times. Reading the less recent value will fix
            // it because it is now actualy the current value.
            index = (index - 1) & (cRingSize-1);
        }
        return value;
    }
    inline void setValue(const T& value)
    {
        // Test if we can read atomic
        // This test is const and will be mad only at compile time
        auto index = size_t{0};
        do {
            index = m_writeIndex.fetch_add(1);
            // This will be repeated if the value is locked
            // 1) by an other writer writing at the same time or
            // 2) a delayed reader is still blocking the formerly current value
            // In both cases writing to the next value will fix it.
        } while (!m_ring[index&(cRingSize-1)].trySet(value));
        m_readIndex = index;
    }
  protected:
    ControlValueAtomicBase()
        : m_readIndex(0),
          m_writeIndex(1)
    {
        // NOTE(rryan): Wrapping max with parentheses avoids conflict with the
        // max macro defined in windows.h.
        DEBUG_ASSERT(((std::numeric_limits<size_t>::max)() & (cRingSize-1)) == (cRingSize - 1));
    }
  private:
    // In worst case, each reader can consume a reader slot from a different ring element.
    // In this case there is still one ring element available for writing.
    ControlRingValue<T> m_ring[cRingSize];
    std::atomic<size_t> m_readIndex{0};
    std::atomic<size_t> m_writeIndex{1};
};
template<typename T>
class ControlValueAtomicBase<T, true> {
public:
    std::atomic<T>  m_value;
    inline T getValue() const
    {
        return m_value.load();
    }
    inline void setValue(const T& value)
    {
        m_value.store(value);
    }
protected:
    ControlValueAtomicBase() = default;
};
// ControlValueAtomic is a wrapper around ControlValueAtomicBase which uses the
// sizeof(T) to determine which underlying implementation of
// ControlValueAtomicBase to use. For types where sizeof(T) <= sizeof(void*),
// the specialized implementation of ControlValueAtomicBase for types that are
// atomic on the architecture is used.

template<typename T>
class ControlValueAtomic : public ControlValueAtomicBase<T, sizeof(T) <= sizeof(long double) > {
  public:
    ControlValueAtomic() : ControlValueAtomicBase<T, sizeof(T) <= sizeof(long double) >() {}
};
