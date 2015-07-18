/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008-2009 Matthew Davies and QMUL.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "DownBeat.h"

#include "MathAliases.h"
#include "MathUtilities.h"
#include "KLDivergence.h"
#include "FFT.h"
#include <iostream>
#include <cstdlib>

DownBeat::DownBeat(float originalSampleRate,
                   size_t decimationFactor,
                   size_t dfIncrement) :
    m_bpb(0),
    m_rate(originalSampleRate),
    m_factor(decimationFactor),
    m_increment(dfIncrement),
    m_decimator1(0),
    m_decimator2(0),
    m_buffer(0),
    m_decbuf(0),
    m_bufsiz(0),
    m_buffill(0),
    m_beatframesize(0),
    m_beatframe(0){
    // beat frame size is next power of two up from 1.3 seconds at the
    // downsampled rate (happens to produce 4096 for 44100 or 48000 at
    // 16x decimation, which is our expected normal situation)
    m_beatframesize = MathUtilities::nextPowerOfTwo (int((m_rate / decimationFactor) * 1.3));
//    std::cerr << "rate = " << m_rate << ", bfs = " << m_beatframesize << std::endl;
    m_beatframe = new float[m_beatframesize];
    m_fftRealOut = new float[m_beatframesize];
    m_fftImagOut = new float[m_beatframesize];
    m_fft = new FFT(m_beatframesize);
}
DownBeat::~DownBeat(){
    delete m_decimator1;
    delete m_decimator2;
    if (m_buffer) free(m_buffer);
    delete[] m_decbuf;
    delete[] m_beatframe;
    delete[] m_fftRealOut;
    delete[] m_fftImagOut;
    delete m_fft;
}
void
DownBeat::setBeatsPerBar(int bpb){m_bpb = bpb;}
void
DownBeat::makeDecimators()
{
//    std::cerr << "m_factor = " << m_factor << std::endl;
    if (m_factor < 2) return;
    auto  highest = Decimator::getHighestSupportedFactor();
    if (m_factor <= highest) {
        m_decimator1 = new Decimator(m_increment, m_factor);
        return;
    }
    m_decimator1 = new Decimator(m_increment, highest);
    m_decimator2 = new Decimator(m_increment / highest, m_factor / highest);
    m_decbuf = new float[m_increment / highest];
}
void
DownBeat::pushAudioBlock(const float *audio)
{
    if (m_buffill + (m_increment / m_factor) > m_bufsiz) {
        if (m_bufsiz == 0) m_bufsiz = m_increment * 16;
        else m_bufsiz = m_bufsiz * 2;
        if (!m_buffer) {
            m_buffer = (decltype(m_buffer))malloc(m_bufsiz * sizeof(float));
        } else {
            m_buffer = (decltype(m_buffer))realloc(m_buffer, m_bufsiz * sizeof(float));
        }
    }
    if (!m_decimator1 && m_factor > 1) makeDecimators();

    if (m_decimator2) {
        m_decimator1->process(audio, m_decbuf);
        m_decimator2->process(m_decbuf, m_buffer + m_buffill);
    } else if (m_decimator1) {
        m_decimator1->process(audio, m_buffer + m_buffill);
    } else {
        // just copy across (m_factor is presumably 1)
        for (size_t i = 0; i < m_increment; ++i) {
            (m_buffer + m_buffill)[i] = audio[i];
        }
    }
    m_buffill += m_increment / m_factor;
}
const float *
DownBeat::getBufferedAudio(size_t &length) const{
    length = m_buffill;
    return m_buffer;
}
void
DownBeat::resetAudioBuffer(){
    if (m_buffer) free(m_buffer);
    m_buffer = 0;
    m_buffill = 0;
    m_bufsiz = 0;
}

void
DownBeat::findDownBeats(const float *audio,
                        size_t audioLength,
                        const d_vec_t &beats,
                        i_vec_t &downbeats)
{
    // FIND DOWNBEATS BY PARTITIONING THE INPUT AUDIO FILE INTO BEAT SEGMENTS
    // WHERE THE AUDIO FRAMES ARE DOWNSAMPLED  BY A FACTOR OF 16 (fs ~= 2700Hz)
    // THEN TAKING THE JENSEN-SHANNON DIVERGENCE BETWEEN BEAT SYNCHRONOUS SPECTRAL FRAMES

    // IMPLEMENTATION (MOSTLY) FOLLOWS:
    //  DAVIES AND PLUMBLEY "A SPECTRAL DIFFERENCE APPROACH TO EXTRACTING DOWNBEATS IN MUSICAL AUDIO"
    //  EUSIPCO 2006, FLORENCE, ITALY

    d_vec_t newspec(m_beatframesize / 2); // magnitude spectrum of current beat
    d_vec_t oldspec(m_beatframesize / 2); // magnitude spectrum of previous beat
    m_beatsd.clear();
    if (audioLength == 0) return;
    for (size_t i = 0; i + 1 < beats.size(); ++i) {
        // Copy the extents of the current beat from downsampled array
        // into beat frame buffer
        auto beatstart = (size_t)((beats[i] * m_increment) / m_factor);
        auto  beatend = (size_t)((beats[i+1] * m_increment) / m_factor);
        beatend = std::max(std::min(beatend,audioLength-1),beatstart);
        auto beatlen = beatend - beatstart;
        // Also apply a Hanning window to the beat frame buffer, sized
        // to the beat extents rather than the frame size.  (Because
        // the size varies, it's easier to do this by hand than use
        // our Window abstraction.)

        for (size_t j = 0; j < beatlen && j < m_beatframesize; ++j) {
            auto mul = 0.5f * (1.0f - std::cos(((float)TWO_PI * (float(j) / float(beatlen)))));
            m_beatframe[j] = audio[beatstart + j] * mul;
        }
        for (size_t j = beatlen; j < m_beatframesize; ++j) {m_beatframe[j] = 0.0;}
        // Now FFT beat frame
        m_fft->process(false, m_beatframe, nullptr,m_fftRealOut, m_fftImagOut);
        // Calculate magnitudes
        for (size_t j = 0; j < m_beatframesize/2; ++j) {
            newspec[j] = hypotf(m_fftRealOut[j], m_fftImagOut[j] );
        }
        // Preserve peaks by applying adaptive threshold
        MathUtilities::adaptiveThreshold(newspec);
        // Calculate JS divergence between new and old spectral frames
        if (i > 0) { // otherwise we have no previous frame
            m_beatsd.push_back(measureSpecDiff(oldspec, newspec));
        }
        // Copy newspec across to old
        for (size_t j = 0; j < m_beatframesize/2; ++j) {oldspec[j] = newspec[j];}
    }
    // We now have all spectral difference measures in specdiff
    int timesig = m_bpb;
    if (timesig == 0) timesig = 4;
    d_vec_t dbcand(timesig); // downbeat candidates
    for (int beat = 0; beat < timesig; ++beat) {dbcand[beat] = 0;}
   // look for beat transition which leads to greatest spectral change
   for (int beat = 0; beat < timesig; ++beat) {
       int count = 0;
       for (int example = beat-1; example < (int)m_beatsd.size(); example += timesig) {
           if (example < 0) continue;
           dbcand[beat] += (m_beatsd[example]) / timesig;
           ++count;
       }
       if (count > 0) dbcand[beat] /= count;
   }
    // first downbeat is beat at index of maximum value of dbcand
    int dbind = MathUtilities::getMax(dbcand);
    // remaining downbeats are at timesig intervals from the first
    for (int i = dbind; i < (int)beats.size(); i += timesig) {downbeats.push_back(i);}
}
float
DownBeat::measureSpecDiff(d_vec_t oldspec, d_vec_t newspec)
{
    // JENSEN-SHANNON DIVERGENCE BETWEEN SPECTRAL FRAMES
    unsigned int SPECSIZE = 512;   // ONLY LOOK AT FIRST 512 SAMPLES OF SPECTRUM. 
    if (SPECSIZE > oldspec.size()/4) {SPECSIZE = oldspec.size()/4;}
    float SD = 0.;
    float sd1 = 0.;
    float sumnew = 0.;
    float sumold = 0.;
    for (unsigned int i = 0;i < SPECSIZE;i++)
    {
        newspec[i] +=EPS;
        oldspec[i] +=EPS;
        sumnew+=newspec[i];
        sumold+=oldspec[i];
    } 
    auto sumnewinv = 1.f/sumnew;
    auto sumoldinv = 1.f/sumold;
    for (unsigned int i = 0;i < SPECSIZE;i++)
    {
        newspec[i] *= (sumnewinv);
        oldspec[i] *= (sumoldinv);
        // JENSEN-SHANNON CALCULATION
        sd1 = 0.5f*oldspec[i] + 0.5f*newspec[i];	
        SD = SD + (-sd1*logf(sd1)) + (0.5f*(oldspec[i]*logf(oldspec[i]))) + (0.5f*(newspec[i]*logf(newspec[i])));
    }
    return SD;
}
void
DownBeat::getBeatSD(vector<float> &beatsd) const{
    for (int i = 0; i < (int)m_beatsd.size(); ++i) beatsd.push_back(m_beatsd[i]);
}

