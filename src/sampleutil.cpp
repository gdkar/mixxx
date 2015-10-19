// sampleutil.cpp
// Created 10/5/2009 by RJ Ryan (rryan@mit.edu)

#include <cstdlib>
#include <cstring>

#include "sampleutil.h"
#include "util/math.h"
#ifdef __WINDOWS__
#include <QtGlobal>
typedef qint64 int64_t;
typedef qint32 int32_t;
#endif

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <immintrin.h>
#include <emmintrin.h>

void SampleUtil::clear(CSAMPLE* pBuffer, int iNumSamples) {
    // Special case: This works, because the binary representation
    // of 0.0f is 0!
    std::fill(&pBuffer[0],&pBuffer[iNumSamples],0);
    //fill(pBuffer, CSAMPLE_ZERO, iNumSamples);
}
void SampleUtil::fill(CSAMPLE* pBuffer, const CSAMPLE value,int iNumSamples) {
    std::fill(&pBuffer[0], &pBuffer[iNumSamples], value);
}
CSAMPLE SampleUtil::clampSample(CSAMPLE in) {return CSAMPLE_clamp(in);}
CSAMPLE SampleUtil::clampGain(CSAMPLE_GAIN in) {return CSAMPLE_GAIN_clamp(in);}
void SampleUtil::copy(CSAMPLE*  pDest, const CSAMPLE* pSrc,int iNumSamples) {
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
    std::move(&pSrc[0],&pSrc[iNumSamples],&pDest[0]);
}
// static
CSAMPLE* SampleUtil::alloc(int size) {
    // To speed up vectorization we align our sample buffers to 16-byte (128
    // bit) boundaries so that vectorized loops doesn't have to do a serial
    // ramp-up before going parallel.
    //
    // Pointers returned by malloc are aligned for the largest scalar type. On
    // most platforms the largest scalar type is long double (16 bytes).
    // However, on MSVC x86 long double is 8 bytes.
    //
    // On MSVC, we use _aligned_malloc to handle aligning pointers to 16-byte
    // boundaries. On other platforms where long double is 8 bytes this code
    // allocates 16 additional slack bytes so we can adjust the pointer we
    // return to the caller to be 16-byte aligned. We record a pointer to the
    // true start of the buffer in the slack space as well so that we can free
    // it correctly.
    // TODO(XXX): Replace with C++11 aligned_alloc.
    // TODO(XXX): consider 32 byte alignement to optimize for AVX builds 
    return new CSAMPLE[size];
}

void SampleUtil::free(CSAMPLE* pBuffer) {
    // See SampleUtil::alloc() for details
    delete[] pBuffer;
}

// static
void SampleUtil::applyGain(CSAMPLE* pBuffer, const CSAMPLE_GAIN gain,
        const int iNumSamples) {
    if (gain == CSAMPLE_GAIN_ONE) return;
    if (gain == CSAMPLE_GAIN_ZERO) {
        clear(pBuffer, iNumSamples);
        return;
    }
    // note: LOOP VECTORIZED.
    pBuffer = (decltype(pBuffer))__builtin_assume_aligned(pBuffer,16);
    const auto gain_ps = _mm_set1_ps(gain);
    for (auto i = 0; i+3 < iNumSamples; i+=4) {
      *(v4sf*)(pBuffer+i) = _mm_mul_ps(gain_ps,*(v4sf*)(pBuffer+i));
    }
}
// static
void SampleUtil::applyRampingGain(CSAMPLE* pBuffer, const CSAMPLE_GAIN old_gain,
        const CSAMPLE_GAIN new_gain, const int iNumSamples) {
    pBuffer = (decltype(pBuffer))__builtin_assume_aligned(pBuffer,16);
    if (old_gain == CSAMPLE_GAIN_ONE && new_gain == CSAMPLE_GAIN_ONE) {return;}
    if (old_gain == CSAMPLE_GAIN_ZERO && new_gain == CSAMPLE_GAIN_ZERO) {
        clear(pBuffer, iNumSamples);
        return;
    }
    const CSAMPLE_GAIN gain_delta = (new_gain - old_gain) / CSAMPLE_GAIN(iNumSamples / 2);
    if (gain_delta) {
    auto gain_ps = _mm_set_ps(old_gain,old_gain,old_gain+gain_delta,old_gain+gain_delta);
    const auto gain_delta_ps = _mm_set1_ps(gain_delta);
        // note: LOOP VECTORIZED.
        for (auto i = 0; i+3 < iNumSamples ; i+=4) {
          *(v4sf*)(pBuffer+i) = _mm_mul_ps(gain_ps,*(v4sf*)(pBuffer+i));
          gain_ps = _mm_add_ps(gain_ps,gain_delta_ps);
        }
    } else {
        // note: LOOP VECTORIZED.
        const auto gain_ps = _mm_set1_ps(old_gain);
        for (auto i = 0; i +3< iNumSamples; i+=4) {
          *(v4sf*)(pBuffer+i) = _mm_mul_ps(gain_ps,*(v4sf*)(pBuffer+i));
        }
    }
}
// static
void SampleUtil::applyAlternatingGain(CSAMPLE* pBuffer, const CSAMPLE_GAIN gain1,
        const CSAMPLE_GAIN gain2, const int iNumSamples) {
    pBuffer = (decltype(pBuffer))__builtin_assume_aligned(pBuffer,16);
    // This handles gain1 == CSAMPLE_GAIN_ONE && gain2 == CSAMPLE_GAIN_ONE as well.
    if (gain1 == gain2) {return applyGain(pBuffer, gain1, iNumSamples);}
    // note: LOOP VECTORIZED.
    const v4sf gain_ps = _mm_set_ps(gain1,gain2,gain1,gain2);
    for (auto i = 0; i+3 < iNumSamples; i+=4) {
        *(v4sf*)(pBuffer+i) = _mm_mul_ps(gain_ps,*(v4sf*)(pBuffer+i));
    }
}
// static
void SampleUtil::addWithGain(CSAMPLE*  pDest, const CSAMPLE*  pSrc,
        const CSAMPLE_GAIN gain, const int iNumSamples) {
    pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
    pSrc  = (decltype(pSrc ))__builtin_assume_aligned(pSrc ,16);
    if (gain == CSAMPLE_GAIN_ZERO) {return;}
    // note: LOOP VECTORIZED.
    const auto gain_ps = _mm_set1_ps(gain);
    for (auto i = 0; i+3 < iNumSamples; i+=4) {
        *(v4sf*)(pDest+i) = _mm_add_ps(*(v4sf*)(pDest+i),
            _mm_mul_ps(gain_ps,*(v4sf*)(pSrc+i)));
    }
}
void SampleUtil::addWithRampingGain(
    CSAMPLE *pDest,
    const CSAMPLE*pSrc0,
    const CSAMPLE_GAIN gain0in,
    const CSAMPLE_GAIN gain0out,
    const int iBufferSize)
{
  pSrc0 = (decltype(pSrc0))__builtin_assume_aligned(pSrc0,16);
  pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
  if(gain0in||gain0out){
    if(gain0in==gain0out)
      addWithGain(pDest,pSrc0,gain0in,iBufferSize);
    else{
      ssize_t i = 0;
      const auto gain0delta = gain0out-gain0in;
      const auto gain0inc   = gain0delta/(iBufferSize*0.5f);
      v4sf gain0_ps = _mm_set_ps(gain0in,gain0in,gain0in+gain0inc,gain0in+gain0inc);
      const v4sf gain0inc_ps = _mm_set1_ps(gain0inc);
      while(i+15<iBufferSize){
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
      }
      while(i+7<iBufferSize){
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
      }
      while(i+3<iBufferSize){
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
      }
    }
  }
}
// static
void SampleUtil::add2WithGain(CSAMPLE*  pDest, const CSAMPLE*  pSrc1,const CSAMPLE_GAIN gain1, const CSAMPLE* pSrc2, const CSAMPLE_GAIN gain2,
        const int iNumSamples) {
    if (gain1 == CSAMPLE_GAIN_ZERO) {
        return addWithGain(pDest, pSrc2, gain2, iNumSamples);
    } else if (gain2 == CSAMPLE_GAIN_ZERO) {
        return addWithGain(pDest, pSrc1, gain1, iNumSamples);
    }

    // note: LOOP VECTORIZED.
    for (auto i = 0; i < iNumSamples; ++i) {
        pDest[i] += pSrc1[i] * gain1 + pSrc2[i] * gain2;
    }
}

// static
void SampleUtil::add3WithGain(CSAMPLE* pDest, const CSAMPLE* pSrc1,
        CSAMPLE_GAIN gain1, const CSAMPLE* pSrc2, CSAMPLE_GAIN gain2,
        const CSAMPLE*  pSrc3, const CSAMPLE_GAIN gain3, const int iNumSamples) {
    if (gain1 == CSAMPLE_GAIN_ZERO) {
        return add2WithGain(pDest, pSrc2, gain2, pSrc3, gain3, iNumSamples);
    } else if (gain2 == CSAMPLE_GAIN_ZERO) {
        return add2WithGain(pDest, pSrc1, gain1, pSrc3, gain3, iNumSamples);
    } else if (gain3 == CSAMPLE_GAIN_ZERO) {
        return add2WithGain(pDest, pSrc1, gain1, pSrc2, gain2, iNumSamples);
    }
    pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
    pSrc1 = (decltype(pSrc1))__builtin_assume_aligned(pSrc1 ,16);
    pSrc2 = (decltype(pSrc2))__builtin_assume_aligned(pSrc2 ,16);
    pSrc3 = (decltype(pSrc3))__builtin_assume_aligned(pSrc3 ,16);

    // note: LOOP VECTORIZED.
    for (auto i = 0; i < iNumSamples; ++i) {
        pDest[i] += pSrc1[i] * gain1 + pSrc2[i] * gain2 + pSrc3[i] * gain3;
    }
}

// static
void SampleUtil::copyWithGain(CSAMPLE* pDest, const CSAMPLE* pSrc,
        const CSAMPLE_GAIN gain, const int iNumSamples) {
    if (gain == CSAMPLE_GAIN_ONE) {
        copy(pDest, pSrc, iNumSamples);
        return;
    }
    if (gain == CSAMPLE_GAIN_ZERO) {
        clear(pDest, iNumSamples);
        return;
    }

    // note: LOOP VECTORIZED.
    for (auto i = 0; i < iNumSamples; ++i) {
        pDest[i] = pSrc[i] * gain;
    }

    // OR! need to test which fares better
    // copy(pDest, pSrc, iNumSamples);
    // applyGain(pDest, gain);
}

// static
void SampleUtil::copyWithRampingGain(CSAMPLE* pDest, const CSAMPLE* pSrc,
        const CSAMPLE_GAIN old_gain, const CSAMPLE_GAIN new_gain, const int iNumSamples) {
    pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
    pSrc  = (decltype(pSrc ))__builtin_assume_aligned(pSrc ,16);

    if (old_gain == CSAMPLE_GAIN_ONE && new_gain == CSAMPLE_GAIN_ONE) {
        copy(pDest, pSrc, iNumSamples);
        return;
    }
    if (old_gain == CSAMPLE_GAIN_ZERO && new_gain == CSAMPLE_GAIN_ZERO) {
        clear(pDest, iNumSamples);
        return;
    }

    const CSAMPLE_GAIN gain_delta = (new_gain - old_gain)
            / CSAMPLE_GAIN(iNumSamples / 2);
    if (gain_delta) {
        const CSAMPLE_GAIN start_gain = old_gain + gain_delta;
        // note: LOOP VECTORIZED.
        pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
        pSrc =  (decltype(pSrc))__builtin_assume_aligned(pSrc ,16);
        for (auto i = 0; i < iNumSamples / 2; ++i) {
            const CSAMPLE_GAIN gain = start_gain + gain_delta * i;
            pDest[i * 2 + 0] = pSrc[i * 2 + 0] * gain;
            pDest[i * 2 + 1] = pSrc[i * 2 + 1] * gain;
        }
    } else {
        // note: LOOP VECTORIZED.
        //    pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
    pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
    pSrc =  (decltype(pSrc))__builtin_assume_aligned(pSrc ,16);

        for (auto i = 0; i < iNumSamples; ++i) {
            pDest[i] = pSrc[i] * old_gain;
        }
    }

    // OR! need to test which fares better
    // copy(pDest, pSrc, iNumSamples);
    // applyRampingGain(pDest, gain);
}

// static
void SampleUtil::convertS16ToFloat32(CSAMPLE* pDest, const SAMPLE* pSrc,
        const int iNumSamples) {
    // SAMPLE_MIN = -32768 is a valid low sample, whereas SAMPLE_MAX = 32767
    // is the highest valid sample. Note that this means that although some
    // sample values convert to -1.0, none will convert to +1.0.
    DEBUG_ASSERT(-SAMPLE_MIN >= SAMPLE_MAX);
    pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
    pSrc  = (decltype(pSrc ))__builtin_assume_aligned(pSrc ,16);

    const CSAMPLE kConversionFactor = -SAMPLE_MIN;
    // note: LOOP VECTORIZED.
    for (auto i = 0; i < iNumSamples; ++i) {
        pDest[i] = CSAMPLE(pSrc[i]) / kConversionFactor;
    }
}

//static
void SampleUtil::convertFloat32ToS16(SAMPLE* pDest, const CSAMPLE* pSrc,
        const int iNumSamples) {
    DEBUG_ASSERT(-SAMPLE_MIN >= SAMPLE_MAX);
    pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
    pSrc  = (decltype(pSrc ))__builtin_assume_aligned(pSrc ,16);
    const CSAMPLE kConversionFactor = -SAMPLE_MIN;
    for (auto i = 0; i < iNumSamples; ++i) {
        pDest[i] = SAMPLE(pSrc[i] * kConversionFactor);
    }
}

// static
SampleUtil::CLIP_STATUS SampleUtil::sumAbsPerChannel(CSAMPLE* pfAbsL, CSAMPLE* pfAbsR,const CSAMPLE* pBuffer, const int iNumSamples) {
    CSAMPLE fAbsL = CSAMPLE_ZERO;
    CSAMPLE fAbsR = CSAMPLE_ZERO;
    CLIP_STATUS clipl = CLIPPING_NONE;
    CLIP_STATUS clipr = CLIPPING_NONE;
    pfAbsL = (decltype(pfAbsL))__builtin_assume_aligned(pfAbsL,16);
    pfAbsR = (decltype(pfAbsR))__builtin_assume_aligned(pfAbsR,16);
    pBuffer = (decltype(pBuffer))__builtin_assume_aligned(pBuffer,16);
    // note: LOOP VECTORIZED.
    for (auto i = 0; i < iNumSamples / 2; ++i) {
        CSAMPLE absl = std::abs(pBuffer[i * 2]);
        fAbsL += absl;
        clipl= absl > CSAMPLE_PEAK ? CLIPPING_LEFT : clipl;
        CSAMPLE absr = std::abs(pBuffer[i * 2 + 1]);
        fAbsR += absr;
        // Replacing the code with a bool clipped will prevent vetorizing
        clipr = absr > CSAMPLE_PEAK ? CLIPPING_RIGHT : clipr;
    }

    *pfAbsL = fAbsL;
    *pfAbsR = fAbsR;
    return static_cast<CLIP_STATUS>(static_cast<int>(clipl)|static_cast<int>(clipr));
}
// static
void SampleUtil::copyClampBuffer(CSAMPLE* pDest, const CSAMPLE* pSrc,
        const int iNumSamples) {
    // note: LOOP VECTORIZED.
    pDest=(decltype(pDest))__builtin_assume_aligned(pDest,16);
    pSrc =(decltype(pSrc ))__builtin_assume_aligned(pSrc ,16);
    for (int i = 0; i < iNumSamples; ++i)  pDest[i] = clampSample(pSrc[i]);
}
// static
void SampleUtil::interleaveBuffer(CSAMPLE* pDest, const CSAMPLE* pSrc1,const CSAMPLE* pSrc2, const int iNumSamples) {
    // note: LOOP VECTORIZED.
    for (int i = 0; i < iNumSamples; ++i) {
        pDest[2 * i + 0] = pSrc1[i];
        pDest[2 * i + 1] = pSrc2[i];
    }
}
// static
void SampleUtil::deinterleaveBuffer(CSAMPLE* pDest1, CSAMPLE* pDest2,const CSAMPLE* pSrc, const int iNumSamples) {
    // note: LOOP VECTORIZED.
    for (int i = 0; i < iNumSamples; ++i) {
        pDest1[i] = pSrc[i * 2 + 0];
        pDest2[i] = pSrc[i * 2 + 1];
    }
}
// static
void SampleUtil::linearCrossfadeBuffers(CSAMPLE* pDest,
        const CSAMPLE* pSrcFadeOut, const CSAMPLE* pSrcFadeIn,
        const int iNumSamples) {
    pDest=(decltype(pDest))__builtin_assume_aligned(pDest,16);
    pSrcFadeIn  =(decltype(pSrcFadeIn  ))__builtin_assume_aligned(pSrcFadeIn ,16);
    pSrcFadeOut =(decltype(pSrcFadeOut ))__builtin_assume_aligned(pSrcFadeOut ,16);
    const auto cross_inc = CSAMPLE_GAIN_ONE / CSAMPLE_GAIN(iNumSamples / 2);
    // note: LOOP VECTORIZED.
    auto cross_mix_in  = CSAMPLE_GAIN{0};
    auto cross_mix_out = CSAMPLE_GAIN_ONE;
    for (int i = 0; i < iNumSamples / 2; ++i) {
        cross_mix_in += cross_inc;
        cross_mix_out-= cross_inc;
        pDest[i * 2] = pSrcFadeIn[i * 2] * cross_mix_in+ pSrcFadeOut[i * 2] * cross_mix_out;
        pDest[i * 2 + 1] = pSrcFadeIn[i * 2 + 1] * cross_mix_in + pSrcFadeOut[i * 2 + 1] * cross_mix_out;

    }
}
// static
void SampleUtil::mixStereoToMono(CSAMPLE* pDest, const CSAMPLE* pSrc,const int iNumSamples) {
    const auto mixScale = CSAMPLE_GAIN_ONE / (CSAMPLE_GAIN_ONE + CSAMPLE_GAIN_ONE);
    // note: LOOP VECTORIZED
    for (auto  i = 0; i < iNumSamples / 2; ++i) {
        pDest[i * 2] = (pSrc[i * 2] + pSrc[i * 2 + 1]) * mixScale;
        pDest[i * 2 + 1] = pDest[i * 2];
    }
}
// static
void SampleUtil::doubleMonoToDualMono(CSAMPLE* pBuffer, const int numFrames) {
    // backward loop
    int i = numFrames;
    // Unvectorizable Loop

    while (0 < i--) {
        pBuffer[i * 2]     = pBuffer[i];
        pBuffer[i * 2 + 1] = pBuffer[i];

    }
}
// static
void SampleUtil::copyMonoToDualMono(CSAMPLE* pDest, const CSAMPLE* pSrc,int numFrames) {
    // forward loop
    // note: LOOP VECTORIZED
    for (int i = 0; i < numFrames; ++i) {
        pDest[i * 2 + 0] = pSrc[i];
        pDest[i * 2 + 1] = pSrc[i];
    }
}
// static
void SampleUtil::stripMultiToStereo(CSAMPLE* pBuffer, const int numFrames,int numChannels) {
    // forward loop
    for (auto i = 0; i < numFrames; ++i) {
        pBuffer[i * 2 + 0] = pBuffer[i * numChannels + 0];
        pBuffer[i * 2 + 1] = pBuffer[i * numChannels + 1];
    }
}
// static
void SampleUtil::copyMultiToStereo(CSAMPLE* pDest, const CSAMPLE* pSrc, int numFrames, int numChannels) {
    // forward loop
    for (auto i = 0; i < numFrames; ++i) {
        pDest[i * 2 + 0] = pSrc[i * numChannels + 0];
        pDest[i * 2 + 1] = pSrc[i * numChannels + 1];
    }
}
// static
void SampleUtil::reverse(CSAMPLE* pBuffer, const int iNumSamples) {
    for (auto j = 0; j < iNumSamples / 4; ++j) {
        const int endpos = (iNumSamples - 1) - j * 2 ;
        auto temp1 = pBuffer[j * 2 + 0];
        auto temp2 = pBuffer[j * 2 + 1];
        pBuffer[j * 2] = pBuffer[endpos - 1];
        pBuffer[j * 2 + 1] = pBuffer[endpos];
        pBuffer[endpos - 1] = temp1;
        pBuffer[endpos] = temp2;
    }
}
void SampleUtil::copy1WithGain(
    CSAMPLE *pDest,
    const CSAMPLE*pSrc0,
    const CSAMPLE_GAIN gain0,
    const int iBufferSize)
{
  pSrc0 = (decltype(pSrc0))__builtin_assume_aligned(pSrc0,16);
  pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
  if(!gain0){
    std::fill(&pDest[0],&pDest[iBufferSize],0);
  }else{
    auto i = ssize_t{0};
    const auto gain0_ps = _mm_set1_ps(gain0);
    while(i+15<iBufferSize){
      *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
      *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
      *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
      *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
    }
    while(i+7<iBufferSize){
      *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
      *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
    }
    while(i+3<iBufferSize){
      *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
    }
  }
}
void SampleUtil::copy1WithRampingGain(
    CSAMPLE *pDest,
    const CSAMPLE*pSrc0,
    const CSAMPLE_GAIN gain0in,
    const CSAMPLE_GAIN gain0out,
    const int iBufferSize)
{
  pSrc0 = (decltype(pSrc0))__builtin_assume_aligned(pSrc0,16);
  pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
  if(!(gain0out||gain0in)){
    std::fill(&pDest[0],&pDest[iBufferSize],0);
  }else{
    if(gain0in==gain0out)copy1WithGain(pDest,pSrc0,gain0in,iBufferSize);
    else{
      ssize_t i = 0;
      const auto gain0delta = gain0out-gain0in;
      const auto gain0inc   = gain0delta/(iBufferSize*0.5f);
      v4sf gain0_ps = _mm_set_ps(gain0in,gain0in,gain0in+gain0inc,gain0in+gain0inc);
      const v4sf gain0inc_ps = _mm_set1_ps(gain0inc);
      while(i+15<iBufferSize){
        *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
      }
      while(i+7<iBufferSize){
        *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
      }
      while(i+3<iBufferSize){
        *(v4sf*)(pDest+i)    = _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
      }
    }
  }
}
void SampleUtil::copy1WithRampingGainAdding(
    CSAMPLE *pDest,
    const CSAMPLE*pSrc0,
    const CSAMPLE_GAIN gain0in,
    const CSAMPLE_GAIN gain0out,
    const int iBufferSize)
{
  pSrc0 = (decltype(pSrc0))__builtin_assume_aligned(pSrc0,16);
  pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
  if(gain0in||gain0out){
    if(gain0in==gain0out){
      copy1WithGainAdding(pDest,pSrc0,gain0in,iBufferSize);
    }else{
      ssize_t i = 0;
      const auto gain0delta = gain0out-gain0in;
      const auto gain0inc   = gain0delta/(iBufferSize*0.5f);
      v4sf gain0_ps = _mm_set_ps(gain0in,gain0in,gain0in+gain0inc,gain0in+gain0inc);
      const v4sf gain0inc_ps = _mm_set1_ps(gain0inc);
      while(i+15<iBufferSize){
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
      }
      while(i+7<iBufferSize){
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
      }
      while(i+3<iBufferSize){
        *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
        gain0_ps = _mm_add_ps(gain0_ps,gain0inc_ps);
        i+=4;
      }
    }
  }
}
void SampleUtil::copy1WithGainAdding(
    CSAMPLE *pDest,
    const CSAMPLE*pSrc0,
    const CSAMPLE_GAIN gain0,
    const int iBufferSize)
{
  pSrc0 = (decltype(pSrc0))__builtin_assume_aligned(pSrc0,16);
  pDest = (decltype(pDest))__builtin_assume_aligned(pDest,16);
  if(gain0){
    ssize_t i = 0;
    const auto gain0_ps = _mm_set1_ps(gain0);
    while(i+15<iBufferSize){
      *(v4sf*)(pDest+i)   += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
      *(v4sf*)(pDest+i)   += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
      *(v4sf*)(pDest+i)   += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
      *(v4sf*)(pDest+i)   += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;

    }
    while(i+7<iBufferSize){
      *(v4sf*)(pDest+i)   += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
      *(v4sf*)(pDest+i)   += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
    }
    while(i+3<iBufferSize){
      *(v4sf*)(pDest+i)    += _mm_mul_ps(gain0_ps,*(v4sf*)(pSrc0+i));
      i+=4;
    }
  }
}
/* static */
void SampleUtil::copy2WithGain(CSAMPLE *pDest,
    const CSAMPLE*pSrc0,const CSAMPLE_GAIN gain0,
    const CSAMPLE*pSrc1,const CSAMPLE_GAIN gain1,
    const int iBufferSize)
{
  copy1WithGain(pDest,pSrc0,gain0,iBufferSize);
  copy1WithGainAdding(pDest,pSrc1,gain1,iBufferSize);
}
void SampleUtil::copy3WithGain(CSAMPLE *pDest,
    const CSAMPLE*pSrc0,const CSAMPLE_GAIN gain0,
    const CSAMPLE*pSrc1,const CSAMPLE_GAIN gain1,
    const CSAMPLE*pSrc2,const CSAMPLE_GAIN gain2,
    const int iBufferSize)
{
  copy1WithGain(pDest,pSrc0,gain0,iBufferSize);
  copy1WithGainAdding(pDest,pSrc1,gain1,iBufferSize);
  copy1WithGainAdding(pDest,pSrc2,gain2,iBufferSize);
}
void SampleUtil::copy4WithGain(CSAMPLE *pDest,
    const CSAMPLE*pSrc0,const CSAMPLE_GAIN gain0,
    const CSAMPLE*pSrc1,const CSAMPLE_GAIN gain1,
    const CSAMPLE*pSrc2,const CSAMPLE_GAIN gain2,
    const CSAMPLE*pSrc3,const CSAMPLE_GAIN gain3,
    const int iBufferSize)
{
  copy1WithGain(pDest,pSrc0,gain0,iBufferSize);
  copy1WithGainAdding(pDest,pSrc1,gain1,iBufferSize);
  copy1WithGainAdding(pDest,pSrc2,gain2,iBufferSize);
  copy1WithGainAdding(pDest,pSrc3,gain3,iBufferSize);
}
void SampleUtil::copy2WithRampingGain(CSAMPLE *pDest,
    const CSAMPLE *pSrc0, const CSAMPLE_GAIN gain0in,const CSAMPLE_GAIN gain0out,
    const CSAMPLE *pSrc1, const CSAMPLE_GAIN gain1in,const CSAMPLE_GAIN gain1out,
    const int iBufferSize)
{
  copy1WithRampingGain(pDest,pSrc0,gain0in,gain0out,iBufferSize);
  copy1WithRampingGainAdding(pDest,pSrc1,gain1in,gain1out,iBufferSize);
}
void SampleUtil::copy3WithRampingGain(CSAMPLE *pDest,
    const CSAMPLE *pSrc0, const CSAMPLE_GAIN gain0in,const CSAMPLE_GAIN gain0out,
    const CSAMPLE *pSrc1, const CSAMPLE_GAIN gain1in,const CSAMPLE_GAIN gain1out,
    const CSAMPLE *pSrc2, const CSAMPLE_GAIN gain2in,const CSAMPLE_GAIN gain2out,
    const int iBufferSize)
{
  copy1WithRampingGain(pDest,pSrc0,gain0in,gain0out,iBufferSize);
  copy1WithRampingGainAdding(pDest,pSrc1,gain1in,gain1out,iBufferSize);
  copy1WithRampingGainAdding(pDest,pSrc2,gain2in,gain2out,iBufferSize);
}
void SampleUtil::copy4WithRampingGain(CSAMPLE *pDest,
    const CSAMPLE *pSrc0, const CSAMPLE_GAIN gain0in,const CSAMPLE_GAIN gain0out,
    const CSAMPLE *pSrc1, const CSAMPLE_GAIN gain1in,const CSAMPLE_GAIN gain1out,
    const CSAMPLE *pSrc2, const CSAMPLE_GAIN gain2in,const CSAMPLE_GAIN gain2out,
    const CSAMPLE *pSrc3, const CSAMPLE_GAIN gain3in,const CSAMPLE_GAIN gain3out,
    const int iBufferSize)
{
  copy1WithRampingGain(pDest,pSrc0,gain0in,gain0out,iBufferSize);
  copy1WithRampingGainAdding(pDest,pSrc1,gain1in,gain1out,iBufferSize);
  copy1WithRampingGainAdding(pDest,pSrc2,gain2in,gain2out,iBufferSize);
  copy1WithRampingGainAdding(pDest,pSrc3,gain3in,gain3out,iBufferSize);
}
