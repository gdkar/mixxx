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

#include "DetectionFunction.h"
#include <cstring>
#include <cmath>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DetectionFunction::DetectionFunction( DFConfig Config ) :
    m_window(0)
{
    m_magHistory = NULL;
    m_phaseHistory = NULL;
    m_phaseHistoryOld = NULL;
    m_magPeaks = NULL;

    initialise( Config );
}

DetectionFunction::~DetectionFunction()
{
    deInitialise();
}


void DetectionFunction::initialise( DFConfig Config )
{
    m_dataLength = Config.frameLength;
    m_halfLength = m_dataLength/2;

    m_DFType = Config.DFType;
    m_stepSize = Config.stepSize;

    m_whiten = Config.adaptiveWhitening;
    m_whitenRelaxCoeff = Config.whiteningRelaxCoeff;
    m_whitenFloor = Config.whiteningFloor;
    if (m_whitenRelaxCoeff < 0) m_whitenRelaxCoeff = 0.9997;
    if (m_whitenFloor < 0) m_whitenFloor = 0.01;

    m_magHistory = new float[ m_halfLength ];
    memset(m_magHistory,0, m_halfLength*sizeof(float));
		
    m_phaseHistory = new float[ m_halfLength ];
    memset(m_phaseHistory,0, m_halfLength*sizeof(float));

    m_phaseHistoryOld = new float[ m_halfLength ];
    memset(m_phaseHistoryOld,0, m_halfLength*sizeof(float));

    m_magPeaks = new float[ m_halfLength ];
    memset(m_magPeaks,0, m_halfLength*sizeof(float));

    // See note in process(const float *) below
    int actualLength = MathUtilities::previousPowerOfTwo(m_dataLength);
    m_phaseVoc = new PhaseVocoder(actualLength);

    m_DFWindowedFrame = new float[ m_dataLength ];
    m_magnitude = new float[ m_halfLength ];
    m_thetaAngle = new float[ m_halfLength ];

    m_window = new Window<float>(HanningWindow, m_dataLength);
}

void DetectionFunction::deInitialise()
{
    delete [] m_magHistory ;
    delete [] m_phaseHistory ;
    delete [] m_phaseHistoryOld ;
    delete [] m_magPeaks ;

    delete m_phaseVoc;

    delete [] m_DFWindowedFrame;
    delete [] m_magnitude;
    delete [] m_thetaAngle;

    delete m_window;
}

float DetectionFunction::process( const float *TDomain )
{
    m_window->cut( TDomain, m_DFWindowedFrame );
    // Our own FFT implementation supports power-of-two sizes only.
    // If we have to use this implementation (as opposed to the
    // version of process() below that operates on frequency domain
    // data directly), we will have to use the next smallest power of
    // two from the block size.  Results may vary accordingly!

    int actualLength = MathUtilities::previousPowerOfTwo(m_dataLength);
    if (actualLength != m_dataLength) {
        // Pre-fill mag and phase vectors with zero, as the FFT output
        // will not fill the arrays
        for (int i = actualLength/2; i < m_dataLength/2; ++i) {
            m_magnitude[i] = 0;
            m_thetaAngle[0] = 0;
        }
    }
    m_phaseVoc->process(m_DFWindowedFrame, m_magnitude, m_thetaAngle);
    if (m_whiten) whiten();
    return runDF();
}

float DetectionFunction::process( const float *magnitudes, const float *phases )
{
    for (size_t i = 0; i < m_halfLength; ++i) {
        m_magnitude[i] = magnitudes[i];
        m_thetaAngle[i] = phases[i];
    }
    if (m_whiten) whiten();
    return runDF();
}

void DetectionFunction::whiten()
{
    for (unsigned int i = 0; i < m_halfLength; ++i) {
        float m = m_magnitude[i];
        if (m < m_magPeaks[i]) {
            m = m + (m_magPeaks[i] - m) * m_whitenRelaxCoeff;
        }
        if (m < m_whitenFloor) m = m_whitenFloor;
        m_magPeaks[i] = m;
        m_magnitude[i] /= m;
    }
}

float DetectionFunction::runDF()
{
    float retVal = 0;

    switch( m_DFType )
    {
    case DF_HFC:
	retVal = HFC( m_halfLength, m_magnitude);
	break;
	
    case DF_SPECDIFF:
	retVal = specDiff( m_halfLength, m_magnitude);
	break;
	
    case DF_PHASEDEV:
	retVal = phaseDev( m_halfLength, m_thetaAngle);
	break;
	
    case DF_COMPLEXSD:
	retVal = complexSD( m_halfLength, m_magnitude, m_thetaAngle);
	break;

    case DF_BROADBAND:
        retVal = broadband( m_halfLength, m_magnitude);
        break;
    }
	
    return retVal;
}

float DetectionFunction::HFC(unsigned int length, float *src)
{
    float val = 0;
    for(auto i = 0; i < length; i++)
    {
	val += src[ i ] * ( i + 1);
    }
    return val;
}

float DetectionFunction::specDiff(unsigned int length, float *src)
{
    float val = 0.0;

    for(auto i = 0; i < length; i++)
    {
	auto temp =  std::abs( (src[ i ] * src[ i ]) - (m_magHistory[ i ] * m_magHistory[ i ]) );
	auto diff= std::sqrt(temp);
        // (See note in phaseDev below.)
        val += diff;
	m_magHistory[ i ] = src[ i ];
    }
    return val;
}
float DetectionFunction::phaseDev(unsigned int length, float *srcPhase)
{
    float val = 0;
    for(auto i = 0; i < length; i++)
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
        auto tmpVal  = fabsf(dev);
        val += tmpVal ;
	m_phaseHistoryOld[ i ] = m_phaseHistory[ i ] ;
	m_phaseHistory[ i ] = srcPhase[ i ];
    }
    return val;
}
float DetectionFunction::complexSD(unsigned int length, float *srcMagnitude, float *srcPhase)
{
    unsigned int i;
    float val = 0;
    float tmpPhase = 0;
    float tmpReal = 0;
    float tmpImag = 0;
   
    float dev = 0;
    ComplexData meas = ComplexData( 0, 0 );
    ComplexData j = ComplexData( 0, 1 );
    for( i = 0; i < length; i++){
	tmpPhase = (srcPhase[ i ]- 2*m_phaseHistory[ i ]+m_phaseHistoryOld[ i ]);
	dev= MathUtilities::princarg( tmpPhase );
	meas = m_magHistory[i] - ( srcMagnitude[ i ] * exp( j * dev) );
	tmpReal = real( meas );
	tmpImag = imag( meas );
	val += hypotf( tmpReal,tmpImag);
	m_phaseHistoryOld[ i ] = m_phaseHistory[ i ] ;
	m_phaseHistory[ i ] = srcPhase[ i ];
	m_magHistory[ i ] = srcMagnitude[ i ];
    }
    return val;
}
float DetectionFunction::broadband(unsigned int length, float *src)
{
    float val = 0;
    for (unsigned int i = 0; i < length; ++i) {
        float sqrmag = src[i] * src[i];
        if (m_magHistory[i] > 0.0) {
            float diff = 10.0 * log10(sqrmag / m_magHistory[i]);
            if (diff > m_dbRise) val = val + 1;
        }
        m_magHistory[i] = sqrmag;
    }
    return val;
}        

float* DetectionFunction::getSpectrumMagnitude()
{
    return m_magnitude;
}

