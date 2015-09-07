#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

#include "util/types.h"
#include <memory>
#include <algorithm>
#include <algorithm> // std::swap

// A sample buffer with properly aligned memory to enable SSE optimizations.
// After construction the content of the buffer is uninitialized. No resize
// operation is provided intentionally because malloc might block!
//
// Hint: If the size of an existing sample buffer ever needs to be altered
// after construction this can simply be achieved by swapping the contents
// with a temporary sample buffer that has been constructed with the desired
// size:
//
//     SampleBuffer sampleBuffer(oldSize);
//     ...
//     SampleBuffer tempBuffer(newSize)
//     ... copy data from sampleBuffer to tempBuffer...
//     sampleBuffer.swap(sampleBuffer);
//
// The local variable tempBuffer can be omitted if no data needs to be
// copied from the existing sampleBuffer:
//
//     SampleBuffer sampleBuffer(oldSize);
//     ...
//     SampleBuffer(newSize).swap(sampleBuffer);
//
class SampleBuffer {
  public:
    SampleBuffer() = default;
    explicit SampleBuffer(SINT size);
    SampleBuffer(SampleBuffer&& other) = default;
    SampleBuffer(const SampleBuffer &other ) = delete;
    virtual ~SampleBuffer() = default;
    SampleBuffer& operator=(SampleBuffer&& other) = default;
    SampleBuffer& operator=(const SampleBuffer& other ) = delete;
    inline SINT size() const { return m_size; }
    inline CSAMPLE* data(SINT offset = 0) {
        DEBUG_ASSERT(0 <= offset);
        // >=: allow access to one element behind allocated memory
        DEBUG_ASSERT(m_size >= offset);
        return &m_data[offset];
    }
    inline const CSAMPLE* data(SINT offset = 0) const {
        DEBUG_ASSERT(0 <= offset);
        // >=: allow access to one element behind allocated memory
        DEBUG_ASSERT(m_size >= offset);
        return &m_data[offset];
    }
    inline CSAMPLE& operator[](SINT index) { return *data(index); }
    inline const CSAMPLE& operator[](SINT index) const { return *data(index); }
    inline CSAMPLE at(SINT index){return m_data[index];}
    inline CSAMPLE *begin()const{return m_data.get();}
    inline CSAMPLE *end  ()const{return m_data.get()+m_size;}
    // Exchanges the members of two buffers in conformance with the
    // implementation of all STL containers. Required for exception
    // safe programming and as a workaround for the missing resize
    // operation.
    void swap(SampleBuffer& other) {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
    }
    // Fills the whole buffer with zeroes
    void clear();
    // Fills the whole buffer with the same value
    void fill(CSAMPLE value);
    class ReadableChunk {
      public:
        ReadableChunk(const SampleBuffer& buffer, SINT offset, SINT length)
            : m_data(buffer.data(offset)),
              m_size(length) {
            DEBUG_ASSERT((buffer.size() - offset) >= length);
        }
        const CSAMPLE* data(SINT offset = 0) const {
            DEBUG_ASSERT(0 <= offset);
            // >=: allow access to one element behind allocated memory
            DEBUG_ASSERT(m_size >= offset);
            return m_data + offset;
        }
        SINT size() const { return m_size; }
        const CSAMPLE& operator[](SINT index) const { return *data(index); }
      private:
        const CSAMPLE* m_data = nullptr;
        SINT m_size = 0;
    };
    class WritableChunk {
      public:
        WritableChunk(SampleBuffer& buffer, SINT offset, SINT length)
            : m_data(buffer.data(offset)),
              m_size(length) {
            DEBUG_ASSERT((buffer.size() - offset) >= length);
        }
        CSAMPLE* data(SINT offset = 0) const {
            DEBUG_ASSERT(0 <= offset);
            // >=: allow access to one element behind allocated memory
            DEBUG_ASSERT(m_size >= offset);
            return m_data + offset;
        }
        SINT size() const { return m_size; }
        CSAMPLE& operator[](SINT index) const { return *data(index); }
      private:
        CSAMPLE* m_data = nullptr;
        SINT m_size = 0;
    };
  private:
      std::unique_ptr<CSAMPLE[]> m_data;
      SINT m_size {0};
};
namespace std {
// Template specialization of std::swap for SampleBuffer.
template<>
void swap(SampleBuffer& lhs, SampleBuffer& rhs) noexcept { lhs.swap(rhs); }
}  // namespace std

#endif // SAMPLEBUFFER_H
