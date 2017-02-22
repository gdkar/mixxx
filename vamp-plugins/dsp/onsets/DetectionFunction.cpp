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
#include "maths/MathUtilities.h"
#include "DetectionFunction.h"
#include <cstring>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DetectionFunction::DetectionFunction( DFConfig Config ) :
    m_DFType(Config.DFType)
   ,m_dataLength(Config.frameLength)
   ,m_halfLength(m_dataLength/2+1)
   ,m_stepSize(Config.stepSize)
   ,m_dbRise(Config.dbRise)
   ,m_whiten(Config.adaptiveWhitening)
   ,m_whitenRelaxCoeff(Config.whiteningRelaxCoeff)
   ,m_whitenFloor(Config.whiteningFloor)

{
    if (m_whitenRelaxCoeff < 0)
        m_whitenRelaxCoeff = 0.9997;
    if (m_whitenFloor < 0)
        m_whitenFloor = 0.01;

    m_magHistory = std::make_unique<float[]>( m_halfLength );
    m_phaseHistory = std::make_unique<float[]>( m_halfLength );
    m_phaseHistoryOld = std::make_unique<float[]>( m_halfLength );
    m_magPeaks = std::make_unique<float[]>( m_halfLength );

    m_magnitude = std::make_unique<float[]>( m_halfLength );
    m_thetaAngle = std::make_unique<float[]>( m_halfLength );
    m_unwrapped = std::make_unique<float[]>( m_halfLength );

    m_windowed = std::make_unique<float[]>( m_dataLength );
}
DetectionFunction::~DetectionFunction() = default;

float DetectionFunction::processTimeDomain(const float*samples)
{
    m_window.cut(samples, m_windowed.get());
    m_phaseVoc.processTimeDomain(m_windowed.get(),m_magnitude.get(), m_thetaAngle.get(), m_unwrapped.get());
    if (m_whiten)
        whiten();

    return runDF();
}

float DetectionFunction::processFrequencyDomain(const float *reals,
                                                 const float *imags)
{
    m_phaseVoc.processFrequencyDomain(
          reals
        , imags
        , &m_magnitude[0]
        , &m_thetaAngle[0]
        , &m_unwrapped[0]);
    if (m_whiten)
        whiten();
    return runDF();
}

void DetectionFunction::whiten()
{
    for (auto i = 0ul; i < m_halfLength; ++i) {
        auto m = m_magnitude[i];
        if (m < m_magPeaks[i])
            m = m + (m_magPeaks[i] - m) * m_whitenRelaxCoeff;
        m = std::max(m_whitenFloor,m);
        m_magPeaks[i] = m;
        m_magnitude[i] /= m;
    }
}

float DetectionFunction::runDF()
{
    auto retVal = 0.f;
    switch( m_DFType )
    {
    case DF_HFC:
	retVal = HFC( m_halfLength, m_magnitude.get());
	break;

    case DF_SPECDIFF:
	retVal = specDiff( m_halfLength, m_magnitude.get());
	break;

    case DF_PHASEDEV:
        // Using the instantaneous phases here actually provides the
        // same results (for these calculations) as if we had used
        // unwrapped phases, but without the possible accumulation of
        // phase error over time
	retVal = phaseDev( m_halfLength, m_thetaAngle.get());
	break;

    case DF_COMPLEXSD:
	retVal = complexSD( m_halfLength, m_magnitude.get(), m_thetaAngle.get());
	break;

    case DF_BROADBAND:
        retVal = broadband( m_halfLength, m_magnitude.get());
        break;
    }

    return retVal;
}

float DetectionFunction::HFC(size_t length, float *src)
{
    size_t i;
    auto val = 0.f;

    for( i = 0; i < length; i++) {
	val += src[ i ] * ( i + 1);
    }
    return val;
}

float DetectionFunction::specDiff(size_t length, float *src)
{
    size_t i;
    auto val = 0.0f;
    auto temp = 0.0f;
    auto diff = 0.0f;
    for( i = 0; i < length; i++) {
	temp = std::abs( (src[ i ] * src[ i ]) - (m_magHistory[ i ] * m_magHistory[ i ]) );
	diff= sqrt(temp);
        // (See note in phaseDev below.)
        val += diff;
	m_magHistory[ i ] = src[ i ];
    }
    return val;
}


float DetectionFunction::phaseDev(size_t length, float *srcPhase)
{
    auto val = 0.0f;

    for( auto i = 0ul; i < length; i++)
    {
	auto tmpPhase = (srcPhase[ i ]- 2*m_phaseHistory[ i ]+m_phaseHistoryOld[ i ]);
	auto dev = MathUtilities::princarg( tmpPhase );

        // A previous version of this code only counted the value here
        // if the magnitude exceeded 0.1.  My impression is that
        // doesn't greatly improve the results for "loud" music (so
        // long as the peak picker is reasonably sophisticated), but
        // does significantly damage its ability to work with quieter
        // music, so I'm removing it and counting the result always.
        // Same goes for the spectral difference measure above.

        val +=std::abs(dev);

	m_phaseHistoryOld[ i ] = m_phaseHistory[ i ] ;
	m_phaseHistory[ i ] = srcPhase[ i ];
    }

    return val;
}


float DetectionFunction::complexSD(size_t length, float*srcMagnitude, float *srcPhase)
{
    auto val = 0.0f;
    auto j = std::complex<float>( 0, 1 );

    for( auto i = 0ul; i < length; i++) {
	auto tmpPhase = (srcPhase[ i ]- 2*m_phaseHistory[ i ]+m_phaseHistoryOld[ i ]);
	auto dev= MathUtilities::princarg( tmpPhase );

	auto meas = m_magHistory[i] - ( srcMagnitude[ i ] * std::exp( j * dev) );

	val += std::abs(meas);

	m_phaseHistoryOld[ i ] = m_phaseHistory[ i ] ;
	m_phaseHistory[ i ]    = srcPhase[ i ];
	m_magHistory[ i ]      = srcMagnitude[ i ];
    }

    return val;
}

float DetectionFunction::broadband(size_t length, float *src)
{
    auto val = 0.0f;
    for (auto i = 0ul; i < length; ++i) {
        auto sqrmag = src[i] * src[i];
        if (m_magHistory[i] > 0.0) {
            auto diff = 10.0f * std::log10(sqrmag / m_magHistory[i]);
            val += (diff > m_dbRise);
        }
        m_magHistory[i] = sqrmag;
    }
    return val;
}
float * DetectionFunction::getSpectrumMagnitude()
{
    return m_magnitude.get();
}
