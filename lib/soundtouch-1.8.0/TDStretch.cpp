////////////////////////////////////////////////////////////////////////////////
/// 
/// Sampled sound tempo changer/time stretch algorithm. Changes the sound tempo 
/// while maintaining the original pitch by using a time domain WSOLA-like 
/// method with several performance-increasing tweaks.
///
/// Note : MMX optimized functions reside in a separate, platform-specific 
/// file, e.g. 'mmx_win.cpp' or 'mmx_gcc.cpp'
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2014-04-06 11:57:21 -0400 (Sun, 06 Apr 2014) $
// File revision : $Revision: 1.12 $
//
// $Id: TDStretch.cpp 195 2014-04-06 15:57:21Z oparviai $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include <cstring>
#include <climits>
#include <cassert>
#include <cmath>
#include <cfloat>

#include "STTypes.h"
#include "cpu_detect.h"
#include "TDStretch.h"

using namespace soundtouch;

#define max(x, y) (((x) > (y)) ? (x) : (y))


/*****************************************************************************
 *
 * Constant definitions
 *
 *****************************************************************************/

// Table for the hierarchical mixing position seeking algorithm
static const short _scanOffsets[4][24]={
    { 120,  180,  240,  300,  360,  420,  480,  540,  600,  660,  720, 780,
      840,  900,  960, 1020, 1080, 1140, 1200, 1260, 1320, 1380, 1440,   0},
    { -96,  -72,  -48,  -24,   24,   48,   72,   96,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    { -20,  -16,  -12,   -8,    8,   12,   16,   20,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    {  -4,   -3,   -2,   -1,    1,    2,    3,    4,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0}};

/*****************************************************************************
 *
 * Implementation of the class 'TDStretch'
 *
 *****************************************************************************/


TDStretch::TDStretch() : FIFOProcessor(&outputBuffer)
{
    bQuickSeek = true;
    channels = 2;

    pMidBuffer = nullptr;
    overlapLength = 0;

    bAutoSeqSetting = true;
    bAutoSeekSetting = true;

//    outDebt = 0;
    skipFract = 0;

    tempo = 1.0f;
    setParameters(44100, DEFAULT_SEQUENCE_MS, DEFAULT_SEEKWINDOW_MS, DEFAULT_OVERLAP_MS);
    setTempo(1.0f);

    clear();
}



TDStretch::~TDStretch(){delete[] pMidBuffer;}



// Sets routine control parameters. These control are certain time constants
// defining how the sound is stretched to the desired duration.
//
// 'sampleRate' = sample rate of the sound
// 'sequenceMS' = one processing sequence length in milliseconds (default = 82 ms)
// 'seekwindowMS' = seeking window length for scanning the best overlapping 
//      position (default = 28 ms)
// 'overlapMS' = overlapping length (default = 12 ms)

void TDStretch::setParameters(int aSampleRate, int aSequenceMS, int aSeekWindowMS, int aOverlapMS)
{
    // accept only positive parameter values - if zero or negative, use old values instead
    if (aSampleRate > 0)   this->sampleRate = aSampleRate;
    if (aOverlapMS > 0)    this->overlapMs = aOverlapMS;

    if (aSequenceMS > 0)
    {
        this->sequenceMs = aSequenceMS;
        bAutoSeqSetting = false;
    } 
    else if (aSequenceMS == 0)
    {
        // if zero, use automatic setting
        bAutoSeqSetting = true;
    }

    if (aSeekWindowMS > 0) 
    {
        this->seekWindowMs = aSeekWindowMS;
        bAutoSeekSetting = false;
    } 
    else if (aSeekWindowMS == 0) 
    {
        // if zero, use automatic setting
        bAutoSeekSetting = true;
    }

    calcSeqParameters();

    calculateOverlapLength(overlapMs);

    // set tempo to recalculate 'sampleReq'
    setTempo(tempo);
}



/// Get routine control parameters, see setParameters() function.
/// Any of the parameters to this function can be NULL, in such case corresponding parameter
/// value isn't returned.
void TDStretch::getParameters(int *pSampleRate, int *pSequenceMs, int *pSeekWindowMs, int *pOverlapMs) const
{
    if (pSampleRate)
    {
        *pSampleRate = sampleRate;
    }

    if (pSequenceMs)
    {
        *pSequenceMs = (bAutoSeqSetting) ? (USE_AUTO_SEQUENCE_LEN) : sequenceMs;
    }

    if (pSeekWindowMs)
    {
        *pSeekWindowMs = (bAutoSeekSetting) ? (USE_AUTO_SEEKWINDOW_LEN) : seekWindowMs;
    }

    if (pOverlapMs)
    {
        *pOverlapMs = overlapMs;
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'pInput'
void TDStretch::overlapMono(CSAMPLE *pOutput, const CSAMPLE *pInput) const
{
    int i;
    auto m1 = (CSAMPLE )0;
    auto m2 = 1.f/((CSAMPLE )overlapLength);
    for (i = 0; i < overlapLength ; i ++) 
    {
        pOutput[i] = (pInput[i] * m1 + pMidBuffer[i] * m2 ) * overlapLength;
        m1 += 1;
        m2 -= 1;
    }
}
void TDStretch::clearMidBuffer()
{
  std::memset(pMidBuffer, 0, channels * sizeof(CSAMPLE ) * overlapLength);
}
void TDStretch::clearInput()
{
    inputBuffer.clear();
    clearMidBuffer();
}
// Clears the sample buffers
void TDStretch::clear()
{
    outputBuffer.clear();
    clearInput();
}
// Enables/disables the quick position seeking algorithm. Zero to disable, nonzero
// to enable
void TDStretch::enableQuickSeek(bool enable){bQuickSeek = enable;}
// Returns nonzero if the quick seeking algorithm is enabled.
bool TDStretch::isQuickSeekEnabled() const{return bQuickSeek;}
// Seeks for the optimal overlap-mixing position.
int TDStretch::seekBestOverlapPosition(const CSAMPLE *refPos)
{
    if (bQuickSeek) {
        return seekBestOverlapPositionQuick(refPos);
    } else {
        return seekBestOverlapPositionFull(refPos);
    }
}
// Overlaps samples in 'midBuffer' with the samples in 'pInputBuffer' at position
// of 'ovlPos'.
inline void TDStretch::overlap(CSAMPLE *pOutput, const CSAMPLE *pInput, uint ovlPos) const
{
    if (channels == 1){
        // mono sound.
        overlapMono(pOutput, pInput + ovlPos);
    }else if (channels == 2){
        // stereo sound
        overlapStereo(pOutput, pInput + 2 * ovlPos);
    } else {
        assert(channels > 0);
        overlapMulti(pOutput, pInput + channels * ovlPos);
    }
}
// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
int TDStretch::seekBestOverlapPositionFull(const CSAMPLE *refPos) 
{
    float  corr;
    float  norm;
    auto bestOffs = 0;
    // Scans for the best correlation value by testing each possible position
    // over the permitted range.
    auto bestCorr = calcCrossCorr(refPos, pMidBuffer, norm);
    const auto invSeekLength = 1 / (float)seekLength;
    for (auto i = 1; i < seekLength; i ++) {
        // Calculates correlation value for the mixing position corresponding
        // to 'i'. Now call "calcCrossCorrAccumulate" that is otherwise same as
        // "calcCrossCorr", but saves time by reusing & updating previously stored 
        // "norm" value
        corr = calcCrossCorrAccumulate(refPos + channels * i, pMidBuffer, norm);
        // heuristic rule to slightly favour values close to mid of the range
        auto tmp = (float)(2 * i - seekLength) * invSeekLength;
        corr = ((corr + 0.1) * (1.0 - 0.25 * tmp * tmp));
        // Checks for the highest correlation value
        if (corr > bestCorr) {
            bestCorr = corr;
            bestOffs = i;
        }
    }
    // clear cross correlation routine state if necessary (is so e.g. in MMX routines).
    clearCrossCorrState();
    return bestOffs;
}
// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
int TDStretch::seekBestOverlapPositionQuick(const CSAMPLE *refPos) 
{
    auto bestCorr = -FLT_MAX;
    auto bestOffs = _scanOffsets[0][0];
    auto corrOffset = 0;
    auto tempOffset = 0;
    // Scans for the best correlation value using four-pass hierarchical search.
    //
    // The look-up table 'scans' has hierarchical position adjusting steps.
    // In first pass the routine searhes for the highest correlation with 
    // relatively coarse steps, then rescans the neighbourhood of the highest
    // correlation with better resolution and so on.
    for (auto scanCount = 0;scanCount < 4; scanCount ++) {
        for (auto j = 0;_scanOffsets[scanCount][j];j++) {
            float norm;
            tempOffset = corrOffset + _scanOffsets[scanCount][j];
            if (tempOffset >= seekLength) break;
            // Calculates correlation value for the mixing position corresponding
            // to 'tempOffset'
            auto corr = (float )calcCrossCorr(refPos + channels * tempOffset, pMidBuffer, norm);
            // heuristic rule to slightly favour values close to mid of the range
            float tmp = (float )(2 * tempOffset - seekLength) / seekLength;
            corr = ((corr + 0.1) * (1.0 - 0.25 * tmp * tmp));
            // Checks for the highest correlation value
            if (corr > bestCorr) {
                bestCorr = corr;
                bestOffs = tempOffset;
            }
        }
        corrOffset = bestOffs;
    }
    // clear cross correlation routine state if necessary (is so e.g. in MMX routines).
    clearCrossCorrState();
    return bestOffs;
}
/// clear cross correlation routine state if necessary 
void TDStretch::clearCrossCorrState(){}
/// Calculates processing sequence length according to tempo setting
void TDStretch::calcSeqParameters()
{
    // Adjust tempo param according to tempo, so that variating processing sequence length is used
    // at varius tempo settings, between the given low...top limits
    #define AUTOSEQ_TEMPO_LOW   0.5     // auto setting low tempo range (-50%)
    #define AUTOSEQ_TEMPO_TOP   2.0     // auto setting top tempo range (+100%)

    // sequence-ms setting values at above low & top tempo
    #define AUTOSEQ_AT_MIN      125.0
    #define AUTOSEQ_AT_MAX      50.0
    #define AUTOSEQ_K           ((AUTOSEQ_AT_MAX - AUTOSEQ_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
    #define AUTOSEQ_C           (AUTOSEQ_AT_MIN - (AUTOSEQ_K) * (AUTOSEQ_TEMPO_LOW))

    // seek-window-ms setting values at above low & top tempo
    #define AUTOSEEK_AT_MIN     25.0
    #define AUTOSEEK_AT_MAX     15.0
    #define AUTOSEEK_K          ((AUTOSEEK_AT_MAX - AUTOSEEK_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
    #define AUTOSEEK_C          (AUTOSEEK_AT_MIN - (AUTOSEEK_K) * (AUTOSEQ_TEMPO_LOW))

    #define CHECK_LIMITS(x, mi, ma) (((x) < (mi)) ? (mi) : (((x) > (ma)) ? (ma) : (x)))

    float seq, seek;
    if (bAutoSeqSetting){
        seq = AUTOSEQ_C + AUTOSEQ_K * tempo;
        seq = CHECK_LIMITS(seq, AUTOSEQ_AT_MAX, AUTOSEQ_AT_MIN);
        sequenceMs = (int)(seq + 0.5);
    }
    if (bAutoSeekSetting){
        seek = AUTOSEEK_C + AUTOSEEK_K * tempo;
        seek = CHECK_LIMITS(seek, AUTOSEEK_AT_MAX, AUTOSEEK_AT_MIN);
        seekWindowMs = (int)(seek + 0.5);
    }
    // Update seek window lengths
    seekWindowLength = (sampleRate * sequenceMs) / 1000;
    if (seekWindowLength < 2 * overlapLength) 
    {
        seekWindowLength = 2 * overlapLength;
    }
    seekLength = (sampleRate * seekWindowMs) / 1000;
}
// Sets new target tempo. Normal tempo = 'SCALE', smaller values represent slower 
// tempo, larger faster tempo.
void TDStretch::setTempo(float newTempo)
{
    int intskip;
    tempo = newTempo;
    // Calculate new sequence duration
    calcSeqParameters();
    // Calculate ideal skip length (according to tempo value) 
    nominalSkip = tempo * (seekWindowLength - overlapLength);
    intskip = (int)(nominalSkip + 0.5f);
    // Calculate how many samples are needed in the 'inputBuffer' to 
    // process another batch of samples
    //sampleReq = max(intskip + overlapLength, seekWindowLength) + seekLength / 2;
    sampleReq = max(intskip + overlapLength, seekWindowLength) + seekLength;
}
// Sets the number of channels, 1 = mono, 2 = stereo
void TDStretch::setChannels(int numChannels)
{
    assert(numChannels > 0);
    if (channels == numChannels) return;
    channels = numChannels;
    inputBuffer.setChannels(channels);
    outputBuffer.setChannels(channels);
    // re-init overlap/buffer
    overlapLength=0;
    setParameters(sampleRate);
}

// Processes as many processing frames of the samples 'inputBuffer', store
// the result into 'outputBuffer'
void TDStretch::processSamples()
{
    int ovlSkip, offset;
    // Process samples as long as there are enough samples in 'inputBuffer'
    // to form a processing frame.
    while ((int)inputBuffer.size() >= sampleReq) {
        // If tempo differs from the normal ('SCALE'), scan for the best overlapping
        // position
        offset = seekBestOverlapPosition(inputBuffer.begin());
        // Mix the samples in the 'inputBuffer' at position of 'offset' with the 
        // samples in 'midBuffer' using sliding overlapping
        // ... first partially overlap with the end of the previous sequence
        // (that's in 'midBuffer')
        overlap(outputBuffer.end((uint)overlapLength), inputBuffer.begin(), (uint)offset);
        outputBuffer.putSamples((uint)overlapLength);
        // ... then copy sequence samples from 'inputBuffer' to output:
        // length of sequence
        auto temp = (seekWindowLength - 2 * overlapLength);
        // crosscheck that we don't have buffer overflow...
        if ((int)inputBuffer.size() < (offset + temp + overlapLength * 2))
        {
            continue;    // just in case, shouldn't really happen
        }
        outputBuffer.putSamples(inputBuffer.begin() + channels * (offset + overlapLength), (uint)temp);
        // Copies the end of the current sequence from 'inputBuffer' to 
        // 'midBuffer' for being mixed with the beginning of the next 
        // processing sequence and so on
        assert((offset + temp + overlapLength * 2) <= (int)inputBuffer.size());
        std::memmove(pMidBuffer, inputBuffer.begin() + channels * (offset + temp + overlapLength), 
            channels * sizeof(CSAMPLE ) * overlapLength);
        // Remove the processed samples from the input buffer. Update
        // the difference between integer & nominal skip step to 'skipFract'
        // in order to prevent the error from accumulating over time.
        skipFract += nominalSkip;   // real skip size
        ovlSkip = (int)skipFract;   // rounded to integer skip
        skipFract -= ovlSkip;       // maintain the fraction part, i.e. real vs. integer skip
        inputBuffer.receiveSamples((uint)ovlSkip);
    }
}
// Adds 'numsamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void TDStretch::putSamples(const CSAMPLE *samples, uint nSamples)
{
    // Add the samples into the input buffer
    inputBuffer.putSamples(samples, nSamples);
    // Process the samples in input buffer
    processSamples();
}
/// Set new overlap length parameter & reallocate RefMidBuffer if necessary.
void TDStretch::acceptNewOverlapLength(int newOverlapLength)
{
    int prevOvl;
    assert(newOverlapLength >= 0);
    prevOvl = overlapLength;
    overlapLength = newOverlapLength;
    if (overlapLength > prevOvl)
    {
        delete[] pMidBuffer;
        pMidBuffer= reinterpret_cast<CSAMPLE*>(new long double[overlapLength * channels *sizeof(CSAMPLE) / sizeof(long double)]);
        // ensure that 'pMidBuffer' is aligned to 16 byte boundary for efficiency
        clearMidBuffer();
    }
}
// Operator 'new' is overloaded so that it automatically creates a suitable instance 
// depending on if we've a MMX/SSE/etc-capable CPU available or not.
void * TDStretch::operator new(size_t s){
    uint uExtensions;
    uExtensions = detectCPUextensions();
    // Check if MMX/SSE instruction set extensions supported by CPU
#ifdef SOUNDTOUCH_ALLOW_SSE
    if (uExtensions & SUPPORT_SSE)return ::new TDStretchSSE;
    else
#endif // SOUNDTOUCH_ALLOW_SSE
    return ::new TDStretch;
}
TDStretch * TDStretch::newInstance(){return new TDStretch;}
//////////////////////////////////////////////////////////////////////////////
//
// Integer arithmetics specific algorithm implementations.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// Floating point arithmetics specific algorithm implementations.
//

// Overlaps samples in 'midBuffer' with the samples in 'pInput'
void TDStretch::overlapStereo(float *pOutput, const float *pInput) const
{
    const auto fScale = 1.0f / (float)overlapLength;
    auto f1 = 0.0f;
    auto f2 = 1.0f;
    for (auto i = 0; i < (int)overlapLength ; i ++) 
    {
        pOutput[i*2 + 0] = pInput[i*2 + 0] * f1 + pMidBuffer[i*2 + 0] * f2;
        pOutput[i*2 + 1] = pInput[i*2 + 1] * f1 + pMidBuffer[i*2 + 1] * f2;
        f1 += fScale;
        f2 -= fScale;
    }
}
// Overlaps samples in 'midBuffer' with the samples in 'input'. 
void TDStretch::overlapMulti(float *pOutput, const float *pInput) const
{
    const auto fScale = 1.0f / (float)overlapLength;

    auto f1 = 0.0f;
    auto f2 = 1.0f;
    const auto nch = channels;
    for (auto i = 0; i < overlapLength; i ++)
    {
        // note: Could optimize this slightly by taking into account that always channels > 2
        for (auto c = 0; c < nch; c ++)
        {
            pOutput[i*nch+c] = pInput[i*nch+c] * f1 + pMidBuffer[i*nch+c] * f2;
        }
        f1 += fScale;
        f2 -= fScale;
    }
}


/// Calculates overlapInMsec period length in samples.
void TDStretch::calculateOverlapLength(int overlapInMsec)
{
    int newOvl;
    assert(overlapInMsec >= 0);
    newOvl = (sampleRate * overlapInMsec) / 1000;
    if (newOvl < 16) newOvl = 16;
    // must be divisible by 8
    newOvl -= newOvl % 8;
    acceptNewOverlapLength(newOvl);
}


/// Calculate cross-correlation
float TDStretch::calcCrossCorr(const float *mixingPos, const float *compare, float &norm) const
{
    float corr;
    int i;
    corr = norm = 0;
    // Same routine for stereo and mono. For Stereo, unroll by factor of 2.
    // For mono it's same routine yet unrollsd by factor of 4.
    for (i = 0; i < channels * overlapLength; i += 4) 
    {
        corr += mixingPos[i] * compare[i] +
                mixingPos[i + 1] * compare[i + 1];
        norm += mixingPos[i] * mixingPos[i] + 
                mixingPos[i + 1] * mixingPos[i + 1];
        // unroll the loop for better CPU efficiency:
        corr += mixingPos[i + 2] * compare[i + 2] +
                mixingPos[i + 3] * compare[i + 3];
        norm += mixingPos[i + 2] * mixingPos[i + 2] +
                mixingPos[i + 3] * mixingPos[i + 3];
    }
    return corr / std::sqrt((norm < 1e-9 ? 1.0 : norm));
}


/// Update cross-correlation by accumulating "norm" coefficient by previously calculated value
float TDStretch::calcCrossCorrAccumulate(const float *mixingPos, const float *compare, float &norm) const
{
    auto i    = 1;
    auto corr = 0.f;
    // cancel first normalizer tap from previous round
    for (; i <= channels; i ++){norm -= mixingPos[-i] * mixingPos[-i];}
    // Same routine for stereo and mono. For Stereo, unroll by factor of 2.
    // For mono it's same routine yet unrollsd by factor of 4.
    for (i = 0; i < channels * overlapLength; i += 4) {
        corr += mixingPos[i + 0] * compare[i + 0] +
                mixingPos[i + 1] * compare[i + 1] +
                mixingPos[i + 2] * compare[i + 2] +
                mixingPos[i + 3] * compare[i + 3];
    }
    // update normalizer with last samples of this round
    for (int j = 0; j < channels; j ++){
        i --;
        norm += mixingPos[i] * mixingPos[i];
    }
    return corr / std::sqrt((norm < 1e-9 ? 1.0 : norm));
}
