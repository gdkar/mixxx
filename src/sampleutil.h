// sampleutil.h
// Created 10/5/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include "util/types.h"

#include <algorithm>
#include <cstring> // memset
#include <memory>
// A group of utilities for working with samples.
class SampleUtil {
  public:
    enum CLIP_STATUS
    {
      CLIPPING_NONE = 0,
      CLIPPING_LEFT = 1,
      CLIPPING_RIGHT= 2,
      CLIPPING_BOTH = 3
    };
    // Allocated a buffer of CSAMPLE's with length size. Ensures that the buffer
    // is 16-byte aligned for SSE enhancement.
    static CSAMPLE* alloc(int size);
    // Frees a 16-byte aligned buffer allocated by SampleUtil::alloc()
    static void free(CSAMPLE* pBuffer);
    // Sets every sample in pBuffer to zero
    static void clear(CSAMPLE* pBuffer, int iNumSamples);
    // Sets every sample in pBuffer to value
    static void fill(CSAMPLE* pBuffer, const CSAMPLE value,int iNumSamples);
    // Copies every sample from pSrc to pDest
    static void copy(CSAMPLE*  pDest, const CSAMPLE* pSrc,int iNumSamples);
    // Limits a CSAMPLE value to the valid range [-CSAMPLE_PEAK, CSAMPLE_PEAK]
    static CSAMPLE clampSample(CSAMPLE in);
    // Limits a CSAMPLE_GAIN value to the valid range [CSAMPLE_GAIN_MIN, CSAMPLE_GAIN_MAX]
    static CSAMPLE clampGain(CSAMPLE_GAIN in);
    // Multiply every sample in pBuffer by gain
    static void applyGain(CSAMPLE* pBuffer, const CSAMPLE gain, const int iNumSamples);
    // Copy pSrc to pDest and multiply each sample by a factor of gain.
    // For optimum performance use the in-place function applyGain()
    // if pDest == pSrc!
    static void copyWithGain(CSAMPLE* pDest, const CSAMPLE* pSrc,const CSAMPLE_GAIN gain, const int iNumSamples);
    // Apply a different gain to every other sample.
    static void applyAlternatingGain(CSAMPLE* pBuffer, const CSAMPLE_GAIN gain1,const CSAMPLE_GAIN gain2, const int iNumSamples);
    // Multiply every sample in pBuffer ramping from gain1 to gain2.
    // We use ramping as often as possible to prevent soundwave discontinuities
    // which can cause audible clicks and pops.
    static void applyRampingGain(CSAMPLE* pBuffer, const CSAMPLE_GAIN old_gain,
            const CSAMPLE_GAIN new_gain, const int iNumSamples);
    // Copy pSrc to pDest and ramp gain
    // For optimum performance use the in-place function applyRampingGain()
    // if pDest == pSrc!
    static void copyWithRampingGain(CSAMPLE* pDest, const CSAMPLE* pSrc,
            const CSAMPLE_GAIN old_gain, const CSAMPLE_GAIN new_gain,
            const int iNumSamples);
    // Add each sample of pSrc, multiplied by the gain, to pDest
    static void addWithGain(CSAMPLE* pDest, const CSAMPLE* pSrc,
            const CSAMPLE_GAIN gain, const int iNumSamples);
    // Add each sample of pSrc, multiplied by the gain, to pDest
    static void addWithRampingGain(CSAMPLE* pDest, const CSAMPLE* pSrc,
            const CSAMPLE_GAIN old_gain, const CSAMPLE_GAIN new_gain,
            const int iNumSamples);
    // Add to each sample of pDest, pSrc1 multiplied by gain1 plus pSrc2
    // multiplied by gain2
    static void add2WithGain(CSAMPLE* pDest, const CSAMPLE* pSrc1,
            const CSAMPLE_GAIN gain1, const CSAMPLE* pSrc2, const CSAMPLE_GAIN gain2,
            const int iNumSamples);
    // Add to each sample of pDest, pSrc1 multiplied by gain1 plus pSrc2
    // multiplied by gain2 plus pSrc3 multiplied by gain3
    static void add3WithGain(CSAMPLE* pDest, const CSAMPLE* pSrc1,
            const CSAMPLE_GAIN gain1, const CSAMPLE* pSrc2, const CSAMPLE_GAIN gain2,
            const CSAMPLE* pSrc3, const CSAMPLE_GAIN gain3, const int iNumSamples);
    // Convert and normalize a buffer of SAMPLEs in the range [-SAMPLE_MAX, SAMPLE_MAX]
    // to a buffer of CSAMPLEs in the range [-1.0, 1.0].
    static void convertS16ToFloat32(CSAMPLE* pDest, const SAMPLE* pSrc,const int iNumSamples);
    // Convert and normalize a buffer of CSAMPLEs in the range [-1.0, 1.0]
    // to a buffer of SAMPLEs in the range [-SAMPLE_MAX, SAMPLE_MAX].
    static void convertFloat32ToS16(SAMPLE* pDest, const CSAMPLE* pSrc,const int iNumSamples);
    // For each pair of samples in pBuffer (l,r) -- stores the sum of the
    // absolute values of l in pfAbsL, and the sum of the absolute values of r
    // in pfAbsR.
    // returns true in case of clipping > +-1
    static CLIP_STATUS sumAbsPerChannel(CSAMPLE* pfAbsL, CSAMPLE* pfAbsR,const CSAMPLE* pBuffer, const int iNumSamples);
    // Copies every sample in pSrc to pDest, limiting the values in pDest
    // to the valid range of CSAMPLE. If pDest and pSrc are aliases, will
    // not copy will only clamp. Returns true if any samples in pSrc were
    // outside the valid range of CSAMPLE.
    static void copyClampBuffer(CSAMPLE* pDest, const CSAMPLE* pSrc,const int iNumSamples);

    // Interleave the samples in pSrc1 and pSrc2 into pDest. iNumSamples must be
    // the number of samples in pSrc1 and pSrc2, and pDest must have at least
    // space for iNumSamples*2 samples. pDest must not be an alias of pSrc1 or
    // pSrc2.
    static void interleaveBuffer(CSAMPLE* pDest, const CSAMPLE* pSrc1,const CSAMPLE* pSrc2, const int iNumSamples);

    // Deinterleave the samples in pSrc alternately into pDest1 and
    // pDest2. iNumSamples must be the number of samples in pDest1 and pDest2,
    // and pSrc must have at least iNumSamples*2 samples. Neither pDest1 or
    // pDest2 can be aliases of pSrc.
    static void deinterleaveBuffer(CSAMPLE* pDest1, CSAMPLE* pDest2,const CSAMPLE* pSrc, const int iNumSamples);
    // Crossfade two buffers together and put the result in pDest.  All the
    // buffers must be the same length.  pDest may be an alias of the source
    // buffers.  It is preferable to use the copyWithRamping functions, but
    // sometimes this function is necessary.
    static void linearCrossfadeBuffers(CSAMPLE* pDest,const CSAMPLE* pSrcFadeOut, const CSAMPLE* pSrcFadeIn,const int iNumSamples);
    // Mix a buffer down to mono, putting the result in both of the channels.
    // This uses a simple (L+R)/2 method, which assumes that the audio is
    // "mono-compatible", ie there are no major out-of-phase parts of the signal.
    static void mixStereoToMono(CSAMPLE* pDest, const CSAMPLE* pSrc,const int iNumSamples);
    // In-place doubles the mono samples in pBuffer to dual mono samples.
    // (numFrames) samples will be read from pBuffer
    // (numFrames * 2) samples will be written into pBuffer
    static void doubleMonoToDualMono(CSAMPLE* pBuffer, int numFrames);
    // Copies and doubles the mono samples in pSrc to dual mono samples
    // into pDest.
    // (numFrames) samples will be read from pSrc
    // (numFrames * 2) samples will be written into pDest
    static void copyMonoToDualMono(CSAMPLE* pDest, const CSAMPLE* pSrc,const int numFrames);
    // In-place strips interleaved multi-channel samples in pBuffer with
    // numChannels >= 2 down to stereo samples. Only samples from the first
    // two channels will be read and written. Samples from all other
    // channels are discarded.
    // pBuffer must contain (numFrames * numChannels) samples
    // (numFrames * 2) samples will be written into pBuffer
    static void stripMultiToStereo(CSAMPLE* pBuffer, const int numFrames,const int numChannels);
    // Copies and strips interleaved multi-channel sample data in pSrc with
    // numChannels >= 2 down to stereo samples into pDest. Only samples from
    // the first two channels will be read and written. Samples from all other
    // channels will be ignored.
    // pSrc must contain (numFrames * numChannels) samples
    // (numFrames * 2) samples will be written into pDest
    static void copyMultiToStereo(CSAMPLE* pDest, const CSAMPLE* pSrc,const int numFrames, const int numChannels);
    // reverses stereo sample in place
    static void reverse(CSAMPLE* pBuffer, const int iNumSamples);
    // Include auto-generated methods (e.g. copyXWithGain, copyXWithRampingGain,
    // etc.)
    static void copy1WithGain(CSAMPLE *pDest,
        const CSAMPLE*pSrc0,const CSAMPLE_GAIN gain0,
        const int iBufferSize);
    static void copy1WithGainAdding(CSAMPLE *pDest,
        const CSAMPLE*pSrc0,const CSAMPLE_GAIN gain0,
        const int iBufferSize);
    static void copy1WithRampingGain(CSAMPLE *pDest,
        const CSAMPLE*pSrc0,const CSAMPLE_GAIN gain0in,const CSAMPLE_GAIN gain0out,
        const int iBufferSize);
    static void copy1WithRampingGainAdding(CSAMPLE *pDest,
        const CSAMPLE*pSrc0,const CSAMPLE_GAIN gain0in,const CSAMPLE_GAIN gain0out,
        const int iBufferSize);
    static void copy2WithGain(CSAMPLE *pDest,
        const CSAMPLE*,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,
        const int iBufferSize);
    static void copy2WithRampingGain(CSAMPLE *pDest,
        const CSAMPLE*,const CSAMPLE_GAIN ,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,const CSAMPLE_GAIN ,
        const int iBufferSize);
    static void copy3WithGain(CSAMPLE *pDest,
        const CSAMPLE*,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,
        const int iBufferSize);
    static void copy3WithRampingGain(CSAMPLE *pDest,
        const CSAMPLE*,const CSAMPLE_GAIN ,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,const CSAMPLE_GAIN ,
        const int iBufferSize);
    static void copy4WithGain(CSAMPLE *pDest,
        const CSAMPLE*,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,
        const int iBufferSize);
    static void copy4WithRampingGain(CSAMPLE *pDest,
        const CSAMPLE*,const CSAMPLE_GAIN ,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,const CSAMPLE_GAIN ,
        const CSAMPLE*,const CSAMPLE_GAIN ,const CSAMPLE_GAIN ,
        const int iBufferSize);
};
