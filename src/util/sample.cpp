#include <cstdlib>
#include <cstddef>

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

namespace {

#ifdef __AVX__
constexpr size_t kAlignment = 32;
#else
constexpr size_t kAlignment = 16;
#endif

// TODO() Check if uintptr_t is available on all our build targets and use that
// instead of size_t, we can remove the sizeof(size_t) check than
constexpr bool useAlignedAlloc() {
    // This will work on all targets and compilers.
    // It will return true bot 32 bit builds and false for 64 bit builds
    return alignof(max_align_t) < kAlignment &&
            sizeof(CSAMPLE*) == sizeof(size_t);
}

} // anonymous namespace

// static
CSAMPLE* SampleUtil::alloc(SINT size)
{
    // To speed up vectorization we align our sample buffers to 16-byte (128
    // bit) boundaries on SSE builds and 32-byte (256 bit) on AVX builds so
    // that vectorized loops doesn't have to do a serial ramp-up before going
    // parallel.
    //
    // Pointers returned by malloc are aligned for the largest scalar type. On
    // most platforms the largest scalar type is long double (16 bytes).
    // However, on MSVC x86 long double is 8 bytes.
    // This can be tested via
    // alignof(std::max_align_t)
    //
    // On MSVC, we use _aligned_malloc to handle aligning pointers to 16-byte
    // boundaries. On other platforms where long double is 8 bytes this code
    // allocates 16 additional slack bytes so we can adjust the pointer we
    // return to the caller to be 16-byte aligned. We record a pointer to the
    // true start of the buffer in the slack space as well so that we can free
    // it correctly.
    // TODO(XXX): Replace with C++17 aligned_alloc.
    if (useAlignedAlloc()) {
#ifdef _MSC_VER
        return static_cast<CSAMPLE*>(
                _aligned_malloc(sizeof(CSAMPLE) * size, kAlignment));
#else
        // This block will be only used on non-Windows platforms that don't
        // produce 16-byte aligned pointers via malloc. We allocate 16 bytes of
        // slack space so that we can align the pointer we return to the caller.
        const size_t alignment = kAlignment;
        const size_t unaligned_size = sizeof(CSAMPLE[size]) + alignment;
        void* pUnaligned = std::malloc(unaligned_size);
        if (pUnaligned == NULL) {
            return NULL;
        }
        // Shift
        void* pAligned = (void*)(((size_t)pUnaligned & ~(alignment - 1)) + alignment);
        // Store pointer to the original buffer in the slack space before the
        // shifted pointer.
        *((void**)(pAligned) - 1) = pUnaligned;
        return static_cast<CSAMPLE*>(pAligned);
#endif
    } else {
        // Our platform already produces aligned pointers (or is an exotic target)
        return new CSAMPLE[size];
    }
    // TODO(XXX): Replace with C++11 aligned_alloc.
    // TODO(XXX): consider 32 byte alignement to optimize for AVX builds
    return new CSAMPLE[size];
}

void SampleUtil::free(CSAMPLE* pBuffer)
{
    delete[] pBuffer;
}

void SampleUtil::copyWithGain(CSAMPLE *pDest, const CSAMPLE *pSrc, CSAMPLE_GAIN gain, SINT num)
{
    if(gain == CSAMPLE_GAIN_ONE)
        copy(pDest,pSrc,num);
    else
        std::transform(&pSrc[0],&pSrc[num],&pDest[0],[gain](auto x){return x * gain;});
}
void SampleUtil::copyWithRampingGain(CSAMPLE *pDest, const CSAMPLE *pSrc, CSAMPLE_GAIN gain_pre, CSAMPLE_GAIN gain_post, SINT num)
{
    if(gain_post == gain_pre)
        copyWithGain(pDest,pSrc,gain_pre,num);
    else {
        num >>= 1;
        auto step = (gain_post - gain_pre) / num;
        for(auto i = 0l; i < num; ++i) {
            pDest[i * 2 + 0] = pSrc[i * 2 + 0] * gain_pre;
            pDest[i * 2 + 1] = pSrc[i * 2 + 1] * gain_pre;
            gain_pre += step;
        }
    }
}
void SampleUtil::addWithGain(CSAMPLE *pDest, const CSAMPLE *pSrc, CSAMPLE_GAIN gain, SINT num)
{
    if(gain == CSAMPLE_ONE) {
        for(auto i = 0l; i < num; ++i)
            pDest[i] += pSrc[i];
    }else {
        for(auto i = 0l; i < num; ++i)
            pDest[i] += pSrc[i] * gain;
    }
}
void SampleUtil::addWithRampingGain(CSAMPLE *pDest, const CSAMPLE *pSrc, CSAMPLE_GAIN gain_pre, CSAMPLE_GAIN gain_post, SINT num)
{
    if(gain_post == gain_pre) {
        addWithGain(pDest,pSrc,gain_pre,num);
    } else {
        num /= 2;
        auto step = (gain_post - gain_pre) / num;
        for(auto i = 0l; i < num; ++i) {
            pDest[i * 2 + 0] += pSrc[i * 2 + 0] * gain_pre;
            pDest[i * 2 + 1] += pSrc[i * 2 + 1] * gain_pre;
            gain_pre += step;
        }
    }
}
void SampleUtil::applyGain(CSAMPLE *pSrc, CSAMPLE_GAIN gain, SINT num)
{
    if(gain != CSAMPLE_GAIN_ONE)
        std::transform(pSrc,pSrc + num, pSrc,[gain](auto x){return x * gain;});
}
void SampleUtil::applyRampingGain(CSAMPLE *pBuffer, CSAMPLE_GAIN gain_pre, CSAMPLE_GAIN gain_post, SINT num)
{
    if(gain_post == gain_pre) {
        if(gain_pre) {
            std::transform(pBuffer ,pBuffer+ num, pBuffer,[gain_pre](auto x){return x * gain_pre;});
        }
    } else {
        num >>= 1;
        auto step = (gain_post - gain_pre) / num;
        for(auto i = 0l; i < num; ++i) {
            pBuffer[i * 2 + 0] *= gain_pre;
            pBuffer[i * 2 + 1] *= gain_pre;
            gain_pre += step;
        }
    }

//    copyWithRampingGain(pBuffer,pBuffer,gain_pre, gain_post,num);
}
void SampleUtil::applyRampingAlternatingGain(CSAMPLE *pBuffer, CSAMPLE_GAIN g1,CSAMPLE_GAIN g2,CSAMPLE_GAIN o1, CSAMPLE_GAIN o2,SINT num)
{
    if(g1 == o1 && g2 == o2) {
        applyAlternatingGain(pBuffer, g1, g2, num);
        return;
    } else if(g1==g2 && o1 == o2) {
        applyRampingGain(pBuffer,o1,g1,num);
        return;
    }
    num >>= 1;
    auto s1 = (g1-o1)/num;
    auto s2 = (g2-o2)/num;
    for(auto i = 0l; i < num; ++i) {
        pBuffer[i * 2 + 0] *= o1;o1 += s1;
        pBuffer[i * 2 + 1] *= o2;o2 += s2;
    }

}

// static
void SampleUtil::applyAlternatingGain(CSAMPLE* pBuffer, CSAMPLE gain1,
        CSAMPLE gain2, SINT iNumSamples)
{
    // This handles gain1 == CSAMPLE_GAIN_ONE && gain2 == CSAMPLE_GAIN_ONE as well.
    if (gain1 == gain2 && gain1 == CSAMPLE_GAIN_ONE)
        return;
//        return applyGain(pBuffer, gain1, iNumSamples);
    // note: LOOP VECTORIZED.
    iNumSamples >>= 1;
    for (SINT i = 0l; i < iNumSamples; ++i) {
        pBuffer[i * 2 + 0l] *= gain1;
        pBuffer[i * 2 + 1l] *= gain2;
    }
}
// static
void SampleUtil::convertS16ToFloat32(CSAMPLE*  pDest, const SAMPLE*  pSrc,
        SINT iNumSamples)
{
    // SAMPLE_MIN = -32768 is a valid low sample, whereas SAMPLE_MAX = 32767
    // is the highest valid sample. Note that this means that although some
    // sample values convert to -1.0, none will convert to +1.0.
    static_assert(-SAMPLE_MIN >= SAMPLE_MAX,"-SAMPLE_MIN must be >= SAMPLE_MAX");
    constexpr CSAMPLE kConversionFactor = CSAMPLE{1}/-SAMPLE_MIN;
    // note: LOOP VECTORIZED.
    std::transform(pSrc,pSrc + iNumSamples, pDest,[=](auto x){return x * kConversionFactor;});
}
//static
void SampleUtil::convertFloat32ToS16(SAMPLE* pDest, const CSAMPLE* pSrc,
        SINT iNumSamples) {
    static_assert(-SAMPLE_MIN >= SAMPLE_MAX,"-SAMPLE_MIN must be >= SAMPLE_MAX");
    constexpr CSAMPLE kConversionFactor = -SAMPLE_MIN;
    std::transform(pSrc,pSrc + iNumSamples, pDest,[=](auto x){return SAMPLE(x * kConversionFactor);});
}
// static
SampleUtil::CLIP_FLAGS SampleUtil::sumAbsPerChannel(CSAMPLE* pfAbsL, CSAMPLE* pfAbsR,
        const CSAMPLE* pBuffer, SINT iNumSamples)
{
    auto fAbsL = CSAMPLE_ZERO;
    auto fAbsR = CSAMPLE_ZERO;
    auto clippedL = CSAMPLE_ZERO;
    auto clippedR = CSAMPLE_ZERO;
    // note: LOOP VECTORIZED.
    iNumSamples >>= 1;
    for (auto i = 0l; i < iNumSamples; ++i) {
        auto absL = std::abs(pBuffer[i * 2 + 0l]);
        auto absR = std::abs(pBuffer[i * 2 + 1l]);
        fAbsL += absL;
        fAbsR += absR;
        clippedL += (absL > CSAMPLE_PEAK) ? CSAMPLE_ONE : CSAMPLE_ZERO;
        clippedR += (absR > CSAMPLE_PEAK) ? CSAMPLE_ONE : CSAMPLE_ZERO;
        // Replacing the code with a bool clipped will prevent vetorizing
    }

    *pfAbsL = fAbsL;
    *pfAbsR = fAbsR;
    auto clipping = static_cast<SampleUtil::CLIP_FLAGS>(SampleUtil::CLIPPING_NONE);
    if (clippedL) clipping |= SampleUtil::CLIPPING_LEFT;
    if (clippedR) clipping |= SampleUtil::CLIPPING_RIGHT;
    return clipping;
}
// static
void SampleUtil::copyClampBuffer(CSAMPLE*  pDest, const  CSAMPLE* pSrc,
        SINT iNumSamples)
{
    // note: LOOP VECTORIZED.
    std::transform(pSrc,pSrc + iNumSamples,pDest,clampSample);
}
// static
void SampleUtil::interleaveBuffer(CSAMPLE*  pDest, const CSAMPLE*  pSrc1,
        const CSAMPLE*  pSrc2, SINT iNumSamples) {
    // note: LOOP VECTORIZED.
    for (auto i = 0l; i < iNumSamples; ++i) {
        pDest[2 * i + 0] = pSrc1[i];
        pDest[2 * i + 1] = pSrc2[i];
    }
}
// static
void SampleUtil::deinterleaveBuffer(CSAMPLE* pDest1, CSAMPLE* pDest2,
        const CSAMPLE* pSrc, SINT iNumSamples) {
    // note: LOOP VECTORIZED.
    for (auto i = 0l; i < iNumSamples; ++i) {
        pDest1[i] = pSrc[i * 2 + 0l];
        pDest2[i] = pSrc[i * 2 + 1l];
    }
}
// static
void SampleUtil::linearCrossfadeBuffers(CSAMPLE* pDest,
        const CSAMPLE* pSrcFadeOut, const CSAMPLE* pSrcFadeIn,
        SINT iNumSamples) {
    auto cross_inc = CSAMPLE_GAIN_ONE / CSAMPLE_GAIN(iNumSamples / 2);
    // note: LOOP VECTORIZED.
    iNumSamples >>= 1;
    for (auto i = SINT{}; i < iNumSamples ; ++i) {
        auto cross_mix = cross_inc * i;
        pDest[i * 2] = pSrcFadeIn[i * 2] * cross_mix
                + pSrcFadeOut[i * 2] * (CSAMPLE_GAIN_ONE - cross_mix);
        pDest[i * 2 + 1] = pSrcFadeIn[i * 2 + 1] * cross_mix
                + pSrcFadeOut[i * 2 + 1] * (CSAMPLE_GAIN_ONE - cross_mix);

    }
}
// static
void SampleUtil::mixStereoToMono(CSAMPLE* pDest, const CSAMPLE* pSrc,
        SINT iNumSamples)
{
    constexpr auto mixScale = CSAMPLE_GAIN_ONE / (CSAMPLE_GAIN_ONE + CSAMPLE_GAIN_ONE);
    // note: LOOP VECTORIZED
    for (auto i = SINT{}; i < iNumSamples / 2; ++i) {
        auto val = (pSrc[i * 2 + 0l] + pSrc[i * 2 + 1l]) * mixScale;
        pDest[i * 2 + 0l] = val;
        pDest[i * 2 + 1l] = val;
    }
}
// static
void SampleUtil::doubleMonoToDualMono(CSAMPLE* pBuffer, SINT numFrames)
{
    // backward loop
    auto i = numFrames;
    // Unvectorizable Loop
    while (0 < i--) {
        auto s = pBuffer[i];
        pBuffer[i * 2 + 0] = s;
        pBuffer[i * 2 + 1] = s;
    }
}
// static
void SampleUtil::copyMonoToDualMono(CSAMPLE*  pDest, const CSAMPLE*  pSrc,
        SINT numFrames)
{
    // forward loop
    // note: LOOP VECTORIZED
    for (auto i = SINT{}; i < numFrames; ++i) {
        auto s = pSrc[i];
        pDest[i * 2 + 0] = s;
        pDest[i * 2 + 1] = s;
    }
}
void SampleUtil::addMonoToStereo(CSAMPLE*  pDest, const CSAMPLE*  pSrc,
        SINT numFrames)
{
    // forward loop
    // note: LOOP VECTORIZED
    for (auto i = SINT{}; i < numFrames; ++i) {
        auto s = pSrc[i];
        pDest[i * 2 + 0] += s;
        pDest[i * 2 + 1] += s;
    }
}


// static
void SampleUtil::stripMultiToStereo(CSAMPLE* pBuffer, SINT numFrames,
        SINT numChannels) {
    // forward loop
    for (auto i = SINT{}; i < numFrames; ++i) {
        pBuffer[i * 2 + 0] = pBuffer[i * numChannels];
        pBuffer[i * 2 + 1] = pBuffer[i * numChannels + 1];
    }
}

// static
void SampleUtil::copyMultiToStereo(CSAMPLE*  pDest, const CSAMPLE*  pSrc,
        SINT numFrames, SINT numChannels) {
    // forward loop
    for (auto i = SINT{}; i < numFrames; ++i) {
        pDest[i * 2] = pSrc[i * numChannels];
        pDest[i * 2 + 1] = pSrc[i * numChannels + 1];
    }
}
// static
void SampleUtil::reverse(CSAMPLE* pBuffer, SINT iNumSamples) {
    for (auto j = SINT{}; j < iNumSamples / 4; ++j) {
        auto endpos = (iNumSamples - 1) - j * 2 ;
        auto temp1 = pBuffer[j * 2];
        auto temp2 = pBuffer[j * 2 + 1];
        pBuffer[j * 2] = pBuffer[endpos - 1];
        pBuffer[j * 2 + 1] = pBuffer[endpos];
        pBuffer[endpos - 1] = temp1;
        pBuffer[endpos] = temp2;
    }
}
// static
void SampleUtil::copyReverse(CSAMPLE*  pDest, const CSAMPLE*  pSrc,
        SINT iNumSamples) {
    for (auto j = SINT{}; j < iNumSamples / 2; ++j) {
        const SINT endpos = (iNumSamples - 1) - j * 2;
        pDest[j * 2] = pSrc[endpos - 1];
        pDest[j * 2 + 1] = pSrc[endpos];
    }
}
