/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file 2005-2006 Christian Landone.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef DETECTIONFUNCTION_H
#define DETECTIONFUNCTION_H

#include "maths/MathUtilities.h"
#include "maths/MathAliases.h"
#include "rubberband/dsp/ReFFT.hpp"
#include "rubberband/base/MiniRing.hpp"

enum struct DFType : int {
    NONE = (0),
    HFC = (1),
    SPECDIFF = (2),
    PHASEDEV = (3),
    COMPLEXSD = (4),
    BROADBAND = (5),
    D2PHI     = (6),
    GROUPDELAY= (7),
    SPECDERIV = (8),
};
struct DFConfig{
    size_t stepSize; // DF step in samples
    size_t frameLength; // DF analysis window - usually 2*step. Must be even!
    int DFType; // type of detection function ( see defines )
    float dbRise; // only used for broadband df (and required for it)
    bool adaptiveWhitening; // perform adaptive whitening
    float whiteningRelaxCoeff; // if < 0, a sensible default will be used
    float whiteningFloor; // if < 0, a sensible default will be used
};

class DetectionFunction
{
public:
    float * getSpectrumMagnitude();
    DetectionFunction() = default;
    DetectionFunction( DFConfig Config );
    DetectionFunction(DetectionFunction && ) noexcept = default;
    DetectionFunction&operator=(DetectionFunction && ) noexcept = default;
    virtual ~DetectionFunction();

    /**
     * Process a single time-domain frame of audio, provided as
     * frameLength samples.
     */
    float processTimeDomain(const float* samples);

    /**
     * Process a single frequency-domain frame, provided as
     * frameLength/2+1 real and imaginary component values.
     */
//    float processFrequencyDomain(const float* reals, const float* imags);

private:
    void whiten();
    float runDF();

    float HFC( );
    float specDiff( );
    float phaseDev();
    float groupDelay();
    float specDeriv();
    float d2Phi();
    float complexSD();
    float broadband();

private:

    int     m_DFType{};
    size_t  m_dataLength{};
    size_t  m_halfLength{m_dataLength/2+1};
    size_t  m_stepSize{};
    size_t  m_position{};
    float   m_dbRise{};
    bool    m_whiten{};
    float   m_whitenRelaxCoeff{};
    float   m_whitenFloor{};

//    std::unique_ptr<float[]> m_magHistory{};
//    std::unique_ptr<float[]> m_phaseHistory{};
//    std::unique_ptr<float[]> m_phaseHistoryOld{};

//    std::unique_ptr<float[]> m_windowed{}; // Array for windowed analysis frame
//    std::unique_ptr<float[]> m_magnitude{}; // Magnitude of analysis frame ( frequency domain )
//    std::unique_ptr<float[]> m_thetaAngle{};// Phase of analysis frame ( frequency domain )
//    std::unique_ptr<float[]> m_unwrapped{}; // Unwrapped phase of analysis frame
    std::unique_ptr<float[]> m_magPeaks{};

    RBMixxxVamp::ReFFT                m_fft{};
    RBMixxxVamp::MiniRing<RBMixxxVamp::ReSpectrum> m_spec{8};
};

#endif
