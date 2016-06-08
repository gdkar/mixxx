#ifndef ENCODERCALLBACK_H
#define ENCODERCALLBACK_H

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdio>

class EncoderCallback {
  public:
    // writes to encoded audio to a stream, e.g., a file stream or broadcast stream
    virtual ~EncoderCallback() = default;
    virtual void write(const uint8_t *buf, size_t buflen) = 0;
};

#endif /* ENCODERCALLBACK_H */
