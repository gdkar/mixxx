_Pragma("once")
class EncoderCallback {
  public:
    // writes to encoded audio to a stream, e.g., a file stream or shoutcast stream
    virtual int write(unsigned char *header, int length) = 0;
    static int write_thunk(void *opaque, unsigned char *data, int length)
    {
      return reinterpret_cast<EncoderCallback*>(opaque)->write(data,length);
    }
};
