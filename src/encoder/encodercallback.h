#ifndef ENCODERCALLBACK_H
#define ENCODERCALLBACK_H

class EncoderCallback {
  public:
    // writes to encoded audio to a stream, e.g., a file stream or broadcast stream
    virtual void write(uint8_t *header, uint8_t *body, int headerLen, int bodyLen) = 0;
};

#endif /* ENCODERCALLBACK_H */
