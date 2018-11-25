_Pragma("once")
#include <algorithm>
#include <cstring> // memset

#include <QFlags>

#include "util/types.h"


// A group of utilities for working with samples.
namespace SampleUtil {
    // If more audio channels are added in the future, this can be used
    // as bitflags, e.g CLIPPING_CH3 = 4
    enum CLIP_FLAG {
        CLIPPING_NONE = 0,
        CLIPPING_LEFT = 1,
        CLIPPING_RIGHT = 2,
    };
    Q_DECLARE_FLAGS(CLIP_FLAGS, CLIP_FLAG);

    // The PlayPosition, Loops and Cue Points used in the Database and
    // Mixxx CO interface are expressed as a floating point number of stereo samples.
    // This is some legacy, we cannot easily revert.
    static constexpr double kPlayPositionChannels = 2.0;

    // Allocated a buffer of CSAMPLE's with length size. Ensures that the buffer
    // is 16-byte aligned for SSE enhancement.
    CSAMPLE* alloc(SINT size);
    // Frees a 16-byte aligned buffer allocated by SampleUtil::alloc()
    void free(CSAMPLE* pBuffer);
    // Sets every sample in pBuffer to zero
    inline
    void clear(CSAMPLE* pBuffer, SINT iNumSamples)
    {
        // Special case: This works, because the binary representation
        // of 0.0f is 0!
        std::fill(pBuffer, pBuffer + iNumSamples, 0);
    }
    // Sets every sample in pBuffer to value
    inline
    void fill(CSAMPLE* pBuffer, CSAMPLE value, SINT iNumSamples)
    {
        std::fill(pBuffer, pBuffer + iNumSamples, value);
    }
    inline SINT ceilPlayPosToFrameStart(double playPos, int numChannels)
    {
        return static_cast<SINT>(std::ceil(playPos/numChannels)) * numChannels;
    }
    inline SINT floorPlayPosToFrameStart(double playPos, int numChannels)
    {
        return static_cast<SINT>(std::floor(playPos/numChannels)) * numChannels;
    }
    inline SINT roundPlayPosToFrameStart(double playPos, int numChannels)
    {
        return static_cast<SINT>(playPos/numChannels) * numChannels;
    }
    inline  SINT truncPlayPosToFrameStart(double playPos, int numChannels)
    {
        return static_cast<SINT>(playPos/numChannels) * numChannels;
    }
    // Copies every sample from pSrc to pDest
    inline
    void copy(CSAMPLE*  pDest, const CSAMPLE*  pSrc, SINT iNumSamples)
    {
        // Benchmark results on 32 bit SSE2 Atom Cpu (Linux)
        // memcpy 7263 ns
        // std::copy 9289 ns
        // SampleUtil::copy 6565 ns
        //
        // Benchmark results from a 64 bit i5 Cpu (Linux)
        // memcpy 518 ns
        // std::copy 664 ns
        // SampleUtil::copy 661 ns
        //
        // memcpy() calls __memcpy_sse2() on 64 bit build only
        // (not available on Debian 32 bit builds)
        // However the Debian 32 bit memcpy() uses a SSE version of
        // memcpy() when called directly from Mixxx source but this
        // requires some checks that can be omitted when inlining the
        // following vectorized loop. Btw.: memcpy() calls from the Qt
        // library are not using SSE istructions.
        std::copy(pSrc, pSrc + iNumSamples, pDest);
    }

    // Limits a CSAMPLE value to the valid range [-CSAMPLE_PEAK, CSAMPLE_PEAK]
    constexpr CSAMPLE clampSample(CSAMPLE in)
    {
        return CSAMPLE_clamp(in);
    }

    // Limits a CSAMPLE_GAIN value to the valid range [CSAMPLE_GAIN_MIN, CSAMPLE_GAIN_MAX]
    constexpr CSAMPLE clampGain(CSAMPLE_GAIN in)
    {
        return CSAMPLE_GAIN_clamp(in);
    }

    inline SINT roundPlayPosToFrame(double playPos) {
        return static_cast<SINT>(std::round(playPos/kPlayPositionChannels));
    }

    inline SINT truncPlayPosToFrame(double playPos) {
        return static_cast<SINT>(playPos / kPlayPositionChannels);
    }

    inline SINT floorPlayPosToFrame(double playPos) {
        return static_cast<SINT>(std::floor(playPos / kPlayPositionChannels));

    }

    inline SINT ceilPlayPosToFrame(double playPos) {
        return static_cast<SINT>(std::ceil(playPos / kPlayPositionChannels));
    }

    // Multiply every sample in pBuffer by gain
    void applyGain(CSAMPLE* pBuffer, CSAMPLE gain,
            SINT iNumSamples);

    // Copy pSrc to pDest and multiply each sample by a factor of gain.
    // For optimum performance use the in-place function applyGain()
    // if pDest == pSrc!
    void copyWithGain(CSAMPLE* pDest, const CSAMPLE* pSrc,
            CSAMPLE_GAIN gain, SINT iNumSamples);


    // Apply a different gain to every other sample.
    void applyAlternatingGain(CSAMPLE* pBuffer, CSAMPLE_GAIN gain1,
            CSAMPLE_GAIN gain2, SINT iNumSamples);

    // Multiply every sample in pBuffer ramping from gain1 to gain2.
    // We use ramping as often as possible to prevent soundwave discontinuities
    // which can cause audible clicks and pops.
    void applyRampingGain(CSAMPLE* pBuffer, CSAMPLE_GAIN old_gain,
            CSAMPLE_GAIN new_gain, SINT iNumSamples);

     void applyRampingAlternatingGain(CSAMPLE* pBuffer, CSAMPLE_GAIN g1,CSAMPLE_GAIN g2
     , CSAMPLE_GAIN o1, CSAMPLE_GAIN o2,
            SINT iNumSamples);
   // Copy pSrc to pDest and ramp gain
    // For optimum performance use the in-place function applyRampingGain()
    // if pDest == pSrc!
    void copyWithRampingGain(CSAMPLE* pDest, const CSAMPLE* pSrc,
            CSAMPLE_GAIN old_gain, CSAMPLE_GAIN new_gain,
            SINT iNumSamples);

    // Add each sample of pSrc, multiplied by the gain, to pDest
    void addWithGain(CSAMPLE* pDest, const CSAMPLE* pSrc,
            CSAMPLE_GAIN gain, SINT iNumSamples);

    // Add each sample of pSrc, multiplied by the gain, to pDest
    void addWithRampingGain(CSAMPLE* pDest, const CSAMPLE* pSrc,
            CSAMPLE_GAIN old_gain, CSAMPLE_GAIN new_gain,
            SINT iNumSamples);
    template<class...T>
    inline void addWithGain(CSAMPLE *pDest, SINT iNumSamples,
        const CSAMPLE*pSrc0, CSAMPLE_GAIN gain0, T... args)
    {
        addWithGain(pDest, pSrc0, gain0,iNumSamples);
        addWithGain(pDest, iNumSamples, args...);
    }
    template<>
    inline void addWithGain(CSAMPLE *pDest, SINT iNumSamples,
        const CSAMPLE *pSrc, CSAMPLE_GAIN gain)
    {
        addWithGain(pDest,pSrc,gain,iNumSamples);
    }
    template<class...T>
    inline void copyWithGain(CSAMPLE *pDest,SINT iNumSamples
        , const CSAMPLE *pSrc0, CSAMPLE_GAIN gain0
        , T... args)
    {
        copyWithGain(pDest, pSrc0, gain0, iNumSamples);
        addWithGain(pDest, iNumSamples, args...);
    }

    // Convert and normalize a buffer of SAMPLEs in the range [-SAMPLE_MAX, SAMPLE_MAX]
    // to a buffer of CSAMPLEs in the range [-1.0, 1.0].
    void convertS16ToFloat32(CSAMPLE* pDest, const SAMPLE* pSrc,
            SINT iNumSamples);

    // Convert and normalize a buffer of CSAMPLEs in the range [-1.0, 1.0]
    // to a buffer of SAMPLEs in the range [-SAMPLE_MAX, SAMPLE_MAX].
    void convertFloat32ToS16(SAMPLE* pDest, const CSAMPLE* pSrc,
            SINT iNumSamples);

    // For each pair of samples in pBuffer (l,r) -- stores the sum of the
    // absolute values of l in pfAbsL, and the sum of the absolute values of r
    // in pfAbsR.
    // The return value tells whether there is clipping in pBuffer or not.
    CLIP_FLAGS sumAbsPerChannel(CSAMPLE* pfAbsL, CSAMPLE* pfAbsR,
            const CSAMPLE* pBuffer, SINT iNumSamples);

    // Copies every sample in pSrc to pDest, limiting the values in pDest
    // to the valid range of CSAMPLE. If pDest and pSrc are aliases, will
    // not copy will only clamp. Returns true if any samples in pSrc were
    // outside the valid range of CSAMPLE.
    void copyClampBuffer(CSAMPLE* pDest, const CSAMPLE* pSrc,
            SINT iNumSamples);

    // Interleave the samples in pSrc1 and pSrc2 into pDest. iNumSamples must be
    // the number of samples in pSrc1 and pSrc2, and pDest must have at least
    // space for iNumSamples*2 samples. pDest must not be an alias of pSrc1 or
    // pSrc2.
    void interleaveBuffer(CSAMPLE* pDest, const CSAMPLE* pSrc1,
            const CSAMPLE* pSrc2, SINT iNumSamples);

    // Deinterleave the samples in pSrc alternately into pDest1 and
    // pDest2. iNumSamples must be the number of samples in pDest1 and pDest2,
    // and pSrc must have at least iNumSamples*2 samples. Neither pDest1 or
    // pDest2 can be aliases of pSrc.
    void deinterleaveBuffer(CSAMPLE* pDest1, CSAMPLE* pDest2,
            const CSAMPLE* pSrc, SINT iNumSamples);

    // Crossfade two buffers together and put the result in pDest.  All the
    // buffers must be the same length.  pDest may be an alias of the source
    // buffers.  It is preferable to use the copyWithRamping functions, but
    // sometimes this function is necessary.
    void linearCrossfadeBuffers(CSAMPLE* pDest,
            const CSAMPLE* pSrcFadeOut, const CSAMPLE* pSrcFadeIn,
            SINT iNumSamples);

    // Mix a buffer down to mono, putting the result in both of the channels.
    // This uses a simple (L+R)/2 method, which assumes that the audio is
    // "mono-compatible", ie there are no major out-of-phase parts of the signal.
    void mixStereoToMono(CSAMPLE* pDest, const CSAMPLE* pSrc,
            SINT iNumSamples);

    // In-place doubles the mono samples in pBuffer to dual mono samples.
    // (numFrames) samples will be read from pBuffer
    // (numFrames * 2) samples will be written into pBuffer
    void doubleMonoToDualMono(CSAMPLE* pBuffer, SINT numFrames);

    // Copies and doubles the mono samples in pSrc to dual mono samples
    // into pDest.
    // (numFrames) samples will be read from pSrc
    // (numFrames * 2) samples will be written into pDest
    void copyMonoToDualMono(CSAMPLE* pDest, const CSAMPLE* pSrc,
            SINT numFrames);
    // Copies and doubles the mono samples in pSrc to dual mono samples
    // into pDest.
    // (numFrames) samples will be read from pSrc
    // (numFrames * 2) samples will be written into pDest
    void addMonoToStereo(CSAMPLE* pDest, const CSAMPLE* pSrc,
            SINT numFrames);


    // In-place strips interleaved multi-channel samples in pBuffer with
    // numChannels >= 2 down to stereo samples. Only samples from the first
    // two channels will be read and written. Samples from all other
    // channels are discarded.
    // pBuffer must contain (numFrames * numChannels) samples
    // (numFrames * 2) samples will be written into pBuffer
    void stripMultiToStereo(CSAMPLE* pBuffer, SINT numFrames,
            SINT numChannels);

    // Copies and strips interleaved multi-channel sample data in pSrc with
    // numChannels >= 2 down to stereo samples into pDest. Only samples from
    // the first two channels will be read and written. Samples from all other
    // channels will be ignored.
    // pSrc must contain (numFrames * numChannels) samples
    // (numFrames * 2) samples will be written into pDest
    void copyMultiToStereo(CSAMPLE* pDest, const CSAMPLE* pSrc,
            SINT numFrames, SINT numChannels);

    // reverses stereo sample in place
    void reverse(CSAMPLE* pBuffer, SINT iNumSamples);

    // copy pSrc to pDest and reverses stereo sample order (backward)
    void copyReverse(CSAMPLE*  pDest,
            const CSAMPLE*  pSrc, SINT iNumSamples);


    void addWithGain(
            CSAMPLE**     pDest,
    const CSAMPLE**     pSrc,
            CSAMPLE_GAIN* gain,
            SINT           num,
            SINT           bands);

    void addWithRampingGain(
            CSAMPLE**     pDest,
    const CSAMPLE**     pSrc,
            CSAMPLE_GAIN* gain_pre,
            CSAMPLE_GAIN* gain_post,
            SINT           num,
            SINT           bands);
    inline void copy2WithRampingGain(
        CSAMPLE* pDest
      , const CSAMPLE* pSrc0, CSAMPLE_GAIN old_gain0, CSAMPLE_GAIN new_gain0
      , const CSAMPLE *pSrc1, CSAMPLE_GAIN old_gain1, CSAMPLE_GAIN new_gain1
      , SINT iNumSamples)
    {
        copyWithRampingGain(pDest, pSrc0, old_gain0, new_gain0,iNumSamples);
        addWithRampingGain(pDest, pSrc1, old_gain1, new_gain1, iNumSamples);
    }
    inline void copy3WithGain(
        CSAMPLE* pDest
      , const CSAMPLE* pSrc0, CSAMPLE_GAIN gain0
      , const CSAMPLE *pSrc1, CSAMPLE_GAIN gain1
      , const CSAMPLE *pSrc2, CSAMPLE_GAIN gain2
      , SINT iNumSamples)
    {
        copyWithGain(pDest,iNumSamples,pSrc0,gain0,pSrc1,gain1,pSrc2,gain2);
    }
    inline void copy2WithGain(
        CSAMPLE* pDest
      , const CSAMPLE* pSrc0, CSAMPLE_GAIN gain0
      , const CSAMPLE *pSrc1, CSAMPLE_GAIN gain1
      , SINT iNumSamples)
    {
        copyWithGain(pDest,iNumSamples,pSrc0,gain0,pSrc1,gain1);
    }
    inline void copy3WithRampingGain(
        CSAMPLE* pDest
      , const CSAMPLE* pSrc0, CSAMPLE_GAIN old_gain0, CSAMPLE_GAIN new_gain0
      , const CSAMPLE *pSrc1, CSAMPLE_GAIN old_gain1, CSAMPLE_GAIN new_gain1
      , const CSAMPLE *pSrc2, CSAMPLE_GAIN old_gain2, CSAMPLE_GAIN new_gain2
      , SINT iNumSamples)
    {
        copyWithRampingGain(pDest, pSrc0, old_gain0, new_gain0,iNumSamples);
        addWithRampingGain(pDest, pSrc1, old_gain1, new_gain1,iNumSamples);
        addWithRampingGain(pDest, pSrc2, old_gain2, new_gain2, iNumSamples);
    }
Q_DECLARE_OPERATORS_FOR_FLAGS(CLIP_FLAGS);
};
