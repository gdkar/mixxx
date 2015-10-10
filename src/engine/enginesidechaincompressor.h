_Pragma("once")
#include "util/types.h"

class EngineSideChainCompressor {
  public:
    EngineSideChainCompressor(const char* group);
    virtual ~EngineSideChainCompressor();
    void setParameters(CSAMPLE threshold, CSAMPLE strength, unsigned int attack_time, unsigned int decay_time);
    void setThreshold(CSAMPLE threshold);
    void setStrength(CSAMPLE strength);
    void setAttackTime(unsigned int attack_time);
    void setDecayTime(unsigned int decay_time);
    // Before calling processKey on multiple channels, first call clearKeys to
    // clear state from the last round of compressor gain calculation.
    void clearKeys();
    // Every loop, before calling process, first call processKey to feed
    // the compressor the input key signal.  It is safe to call this function
    // multiple times for multiple keys, however they will not be summed together
    // so compression will not be triggered unless at least one buffer would
    // have triggered alone.
    void processKey(const CSAMPLE* pIn, const int iBufferSize);
    // Calculates a new gain value based on the current compression ratio
    // over the given number of frames and whether the current input is above threshold.
    double calculateCompressedGain(int frames);
  private:
    // Update the attack and decay rates.
    void calculateRates();
    // The current ratio the signal is being compressed.  This is the same as m_strength
    // when the compressor is at maximum engagement (not attacking or decaying).
    CSAMPLE m_compressRatio = 0;
    // True if the input signal is above the threshold.
    bool m_bAboveThreshold = false;
    // The sample value above which the compressor is triggered.
    CSAMPLE m_threshold = 1;
    // The largest ratio the signal can be compressed.
    CSAMPLE m_strength = 0;
    // The length of time, in frames (samples/2), until maximum compression is reached.
    unsigned int m_attackTime = 0;
    // The length of time, in frames, until compression is completely off.
    unsigned int m_decayTime = 0;
    // These are the delta compression values per sample based on the strengths and timings.
    CSAMPLE m_attackPerFrame = 0;
    CSAMPLE m_decayPerFrame = 0;
};
