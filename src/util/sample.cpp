#include <cstdlib>

#include "util/sample.h"
#include "util/math.h"

#ifdef __WINDOWS__
#include <QtGlobal>
typedef qint64 int64_t;
typedef qint32 int32_t;
#endif

// LOOP VECTORIZED below marks the loops that are processed with the 128 bit SSE
// registers as tested with gcc 4.6 and the -ftree-vectorizer-verbose=2 flag on
// an Intel i5 CPU. When changing, be careful to not disturb the vectorization.
// https://gcc.gnu.org/projects/tree-ssa/vectorization.html
// This also utilizes AVX registers wehn compiled for a recent 64 bit CPU
// using scons optimize=native.

// static
CSAMPLE* SampleUtil::alloc(int size)
{
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

void SampleUtil::free(CSAMPLE* pBuffer)
{
    delete[] pBuffer;
}

// static
void SampleUtil::copyWithGain(CSAMPLE *pDest, const CSAMPLE *pSrc, CSAMPLE_GAIN gain, int num)
{
    std::transform(&pSrc[0],&pSrc[num],&pDest[0],[=](CSAMPLE x){return x * gain;});
}
// static
void SampleUtil::copyWithRampingGain(CSAMPLE *pDest, const CSAMPLE *pSrc, CSAMPLE_GAIN gain_pre, CSAMPLE_GAIN gain_post, int num)
{
    if(gain_post == gain_pre)
        copyWithGain(pDest,pSrc,gain_pre,num);
    else {
        auto step = (gain_post - gain_pre) * (2 / num);
        num /= 2;
        for(auto i = 0; i < num; i ++) {
            pDest[i * 2 + 0] = pSrc[i * 2 + 0] * gain_pre;
            pDest[i * 2 + 1] = pSrc[i * 2 + 1] * gain_pre;
            gain_pre += step;
        }
    }
}
// static
void SampleUtil::addWithGain(CSAMPLE *pDest, const CSAMPLE *pSrc, CSAMPLE_GAIN gain, int num)
{
    for(auto i = 0; i < num; i ++ )
        pDest[i] += pSrc[i] * gain;
}
void SampleUtil::addWithRampingGain(CSAMPLE *pDest, const CSAMPLE *pSrc, CSAMPLE_GAIN gain_pre, CSAMPLE_GAIN gain_post, int num)
{
    if(gain_post == gain_pre) {
        addWithGain(pDest,pSrc,gain_pre,num);
    } else {
        auto step = (gain_post - gain_pre) * (2 / num);
        num /= 2;
        for(auto i = 0; i < num; i ++) {
            pDest[i * 2 + 0] += pSrc[i * 2 + 0] * gain_pre;
            pDest[i * 2 + 1] += pSrc[i * 2 + 1] * gain_pre;
            gain_pre += step;
        }
    }
}
void SampleUtil::applyGain(CSAMPLE *pBuffer, CSAMPLE_GAIN gain, int num)
{
    copyWithGain(pBuffer,pBuffer,gain,num);
}
void SampleUtil::applyRampingGain(CSAMPLE *pBuffer, CSAMPLE_GAIN gain_pre, CSAMPLE_GAIN gain_post, int num)
{
    copyWithRampingGain(pBuffer,pBuffer,gain_pre, gain_post,num);
}
// static
void SampleUtil::applyAlternatingGain(CSAMPLE* pBuffer, CSAMPLE gain1,
        CSAMPLE gain2, int iNumSamples)
{
    // This handles gain1 == CSAMPLE_GAIN_ONE && gain2 == CSAMPLE_GAIN_ONE as well.
    if (gain1 == gain2)
        return applyGain(pBuffer, gain1, iNumSamples);
    // note: LOOP VECTORIZED.
    for (int i = 0; i < iNumSamples / 2; ++i) {
        pBuffer[i * 2 + 0] *= gain1;
        pBuffer[i * 2 + 1] *= gain2;
    }
}

// static
void SampleUtil::convertS16ToFloat32(CSAMPLE*  pDest, const SAMPLE*  pSrc,
        int iNumSamples) {
    // SAMPLE_MIN = -32768 is a valid low sample, whereas SAMPLE_MAX = 32767
    // is the highest valid sample. Note that this means that although some
    // sample values convert to -1.0, none will convert to +1.0.
    DEBUG_ASSERT(-SAMPLE_MIN >= SAMPLE_MAX);
    constexpr CSAMPLE kConversionFactor = 1./-SAMPLE_MIN;
    // note: LOOP VECTORIZED.
    std::transform(pSrc,pSrc + iNumSamples, pDest,[=](auto x){return x * kConversionFactor;});
}
//static
void SampleUtil::convertFloat32ToS16(SAMPLE* pDest, const CSAMPLE* pSrc,
        unsigned int iNumSamples) {
    DEBUG_ASSERT(-SAMPLE_MIN >= SAMPLE_MAX);
    constexpr CSAMPLE kConversionFactor = -SAMPLE_MIN;
    std::transform(pSrc,pSrc + iNumSamples, pDest,[=](auto x){return x * kConversionFactor;});
}

// static
SampleUtil::CLIP_FLAGS SampleUtil::sumAbsPerChannel(CSAMPLE* pfAbsL, CSAMPLE* pfAbsR,
        const CSAMPLE* pBuffer, int iNumSamples) {
    CSAMPLE fAbsL = CSAMPLE_ZERO;
    CSAMPLE fAbsR = CSAMPLE_ZERO;
    CSAMPLE clippedL = 0;
    CSAMPLE clippedR = 0;

    // note: LOOP VECTORIZED.
    for (int i = 0; i < iNumSamples / 2; ++i) {
        CSAMPLE absl = fabs(pBuffer[i * 2]);
        fAbsL += absl;
        clippedL += absl > CSAMPLE_PEAK ? 1 : 0;
        CSAMPLE absr = fabs(pBuffer[i * 2 + 1]);
        fAbsR += absr;
        // Replacing the code with a bool clipped will prevent vetorizing
        clippedR += absr > CSAMPLE_PEAK ? 1 : 0;
    }

    *pfAbsL = fAbsL;
    *pfAbsR = fAbsR;
    auto clipping = static_cast<SampleUtil::CLIP_FLAGS>(SampleUtil::CLIPPING_NONE);
    if (clippedL > 0)
        clipping |= SampleUtil::CLIPPING_LEFT;
    if (clippedR > 0)
        clipping |= SampleUtil::CLIPPING_RIGHT;
    return clipping;
}

// static
void SampleUtil::copyClampBuffer(CSAMPLE*  pDest, const  CSAMPLE* pSrc,
        int iNumSamples) {
    // note: LOOP VECTORIZED.
    std::transform(pSrc,pSrc + iNumSamples,pDest,clampSample);
}

// static
void SampleUtil::interleaveBuffer(CSAMPLE*  pDest, const CSAMPLE*  pSrc1,
        const CSAMPLE*  pSrc2, int iNumSamples) {
    // note: LOOP VECTORIZED.
    for (int i = 0; i < iNumSamples; ++i) {
        pDest[2 * i + 0] = pSrc1[i];
        pDest[2 * i + 1] = pSrc2[i];
    }
}

// static
void SampleUtil::deinterleaveBuffer(CSAMPLE* pDest1, CSAMPLE* pDest2,
        const CSAMPLE* pSrc, int iNumSamples) {
    // note: LOOP VECTORIZED.
    for (int i = 0; i < iNumSamples; ++i) {
        pDest1[i] = pSrc[i * 2];
        pDest2[i] = pSrc[i * 2 + 1];
    }
}

// static
void SampleUtil::linearCrossfadeBuffers(CSAMPLE* pDest,
        const CSAMPLE* pSrcFadeOut, const CSAMPLE* pSrcFadeIn,
        int iNumSamples) {
    const CSAMPLE_GAIN cross_inc = CSAMPLE_GAIN_ONE
            / CSAMPLE_GAIN(iNumSamples / 2);
    // note: LOOP VECTORIZED.
    for (int i = 0; i < iNumSamples / 2; ++i) {
        const CSAMPLE_GAIN cross_mix = cross_inc * i;
        pDest[i * 2] = pSrcFadeIn[i * 2] * cross_mix
                + pSrcFadeOut[i * 2] * (CSAMPLE_GAIN_ONE - cross_mix);
        pDest[i * 2 + 1] = pSrcFadeIn[i * 2 + 1] * cross_mix
                + pSrcFadeOut[i * 2 + 1] * (CSAMPLE_GAIN_ONE - cross_mix);

    }
}

// static
void SampleUtil::mixStereoToMono(CSAMPLE* pDest, const CSAMPLE* pSrc,
        int iNumSamples) {
    const CSAMPLE_GAIN mixScale = CSAMPLE_GAIN_ONE
            / (CSAMPLE_GAIN_ONE + CSAMPLE_GAIN_ONE);
    // note: LOOP VECTORIZED
    for (int i = 0; i < iNumSamples / 2; ++i) {
        pDest[i * 2] = (pSrc[i * 2] + pSrc[i * 2 + 1]) * mixScale;
        pDest[i * 2 + 1] = pDest[i * 2];
    }
}

// static
void SampleUtil::doubleMonoToDualMono(CSAMPLE* pBuffer, int numFrames) {
    // backward loop
    int i = numFrames;
    // Unvectorizable Loop
    while (0 < i--) {
        auto s = pBuffer[i];
        pBuffer[i * 2 + 0] = s;
        pBuffer[i * 2 + 1] = s;
    }
}

// static
void SampleUtil::copyMonoToDualMono(CSAMPLE*  pDest, const CSAMPLE*  pSrc,
        int numFrames) {
    // forward loop
    // note: LOOP VECTORIZED
    for (int i = 0; i < numFrames; ++i) {
        auto s = pSrc[i];
        pDest[i * 2 + 0] = s;
        pDest[i * 2 + 1] = s;
    }
}

// static
void SampleUtil::stripMultiToStereo(CSAMPLE* pBuffer, int numFrames,
        int numChannels) {
    // forward loop
    for (int i = 0; i < numFrames; ++i) {
        pBuffer[i * 2 + 0] = pBuffer[i * numChannels];
        pBuffer[i * 2 + 1] = pBuffer[i * numChannels + 1];
    }
}

// static
void SampleUtil::copyMultiToStereo(CSAMPLE*  pDest, const CSAMPLE*  pSrc,
        int numFrames, int numChannels) {
    // forward loop
    for (int i = 0; i < numFrames; ++i) {
        pDest[i * 2] = pSrc[i * numChannels];
        pDest[i * 2 + 1] = pSrc[i * numChannels + 1];
    }
}


// static
void SampleUtil::reverse(CSAMPLE* pBuffer, int iNumSamples) {
    for (int j = 0; j < iNumSamples / 4; ++j) {
        const int endpos = (iNumSamples - 1) - j * 2 ;
        CSAMPLE temp1 = pBuffer[j * 2];
        CSAMPLE temp2 = pBuffer[j * 2 + 1];
        pBuffer[j * 2] = pBuffer[endpos - 1];
        pBuffer[j * 2 + 1] = pBuffer[endpos];
        pBuffer[endpos - 1] = temp1;
        pBuffer[endpos] = temp2;
    }
}
// static
void SampleUtil::copyReverse(CSAMPLE*  pDest, const CSAMPLE*  pSrc,
        int iNumSamples) {
    for (int j = 0; j < iNumSamples / 2; ++j) {
        const int endpos = (iNumSamples - 1) - j * 2;
        pDest[j * 2] = pSrc[endpos - 1];
        pDest[j * 2 + 1] = pSrc[endpos];
    }
}

