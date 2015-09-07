#include "samplebuffer.h"
SampleBuffer::SampleBuffer(SINT size)
        : m_data(std::make_unique<CSAMPLE[]>(size)),
          m_size(m_data ? size : 0) {}
void SampleBuffer::clear() { std::fill(data(),data()+size(),0); }
// Fills the whole buffer with the same value
void SampleBuffer::fill(CSAMPLE value) { std::fill(data(),data()+size(),value); }
