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

#include <iostream>
#include <cmath>
#include "maths/MathUtilities.h"
#include "Chromagram.h"

//----------------------------------------------------------------------------

Chromagram::Chromagram( ChromaConfig Config ) :
    m_skGenerated(false)
{
    initialise( Config );
}

int Chromagram::initialise( ChromaConfig Config )
{
    m_FMin = Config.min;		// min freq
    m_FMax = Config.max;		// max freq
    m_BPO  = Config.BPO;		// bins per octave
    m_normalise = Config.normalise;     // if frame normalisation is required

    // No. of constant Q bins
    m_uK = ( unsigned int ) ceil( m_BPO * log(m_FMax/m_FMin)/log(2.0));

    // Create array for chroma result
    m_chromadata = new float[ m_BPO ];

    // Create Config Structure for ConstantQ operator
    CQConfig ConstantQConfig;

    // Populate CQ config structure with parameters
    // inherited from the Chroma config
    ConstantQConfig.FS	 = Config.FS;
    ConstantQConfig.min = m_FMin;
    ConstantQConfig.max = m_FMax;
    ConstantQConfig.BPO = m_BPO;
    ConstantQConfig.CQThresh = Config.CQThresh;

    // Initialise ConstantQ operator
    m_ConstantQ = new ConstantQ( ConstantQConfig );

    // Initialise working arrays
    m_frameSize = m_ConstantQ->getfftlength();
    m_hopSize = m_ConstantQ->gethop();

    // Initialise FFT object
    m_FFT = new FFTReal(m_frameSize);

    m_FFTRe = new float[ m_frameSize ];
    m_FFTIm = new float[ m_frameSize ];
    m_CQRe  = new float[ m_uK ];
    m_CQIm  = new float[ m_uK ];

    m_window = 0;
    m_windowbuf = 0;

    return 1;
}

Chromagram::~Chromagram()
{
    deInitialise();
}

int Chromagram::deInitialise()
{
    delete[] m_windowbuf;
    delete m_window;

    delete [] m_chromadata;

    delete m_FFT;

    delete m_ConstantQ;

    delete [] m_FFTRe;
    delete [] m_FFTIm;
    delete [] m_CQRe;
    delete [] m_CQIm;
    return 1;
}

//----------------------------------------------------------------------------------
// returns the absolute value of complex number xx + i*yy
float Chromagram::kabs(float xx, float yy)
{
    float ab = sqrt(xx*xx + yy*yy);
    return(ab);
}
//-----------------------------------------------------------------------------------


void Chromagram::unityNormalise(float *src)
{
    float emax;
    {
        auto it = std::minmax_element(src,src + m_BPO);
        emax = *it.second;
    }
    emax = 1.f / emax;
    for( unsigned int i = 0; i < m_BPO; i++ )
    {
        src[i] *= emax;
    }
}


float* Chromagram::process( const float *data )
{
    if (!m_skGenerated) {
        // Generate CQ Kernel
        m_ConstantQ->sparsekernel();
        m_skGenerated = true;
    }

    if (!m_window) {
        m_window = new Window<float>(HammingWindow, m_frameSize);
        m_windowbuf = new float[m_frameSize];
    }

    for (int i = 0; i < m_frameSize; ++i) {
        m_windowbuf[i] = data[i];
    }
    m_window->cut(m_windowbuf);

    m_FFT->forward(m_windowbuf, m_FFTRe, m_FFTIm);

    return process(m_FFTRe, m_FFTIm);
}

float* Chromagram::process( const float *real, const float *imag )
{
    if (!m_skGenerated) {
        // Generate CQ Kernel
        m_ConstantQ->sparsekernel();
        m_skGenerated = true;
    }

    // initialise chromadata to 0
    for (unsigned i = 0; i < m_BPO; i++) m_chromadata[i] = 0;

    float cmax = 0.0;
    float cval = 0;
    // Calculate ConstantQ frame
    m_ConstantQ->process( real, imag, m_CQRe, m_CQIm );

    // add each octave of cq data into Chromagram
    const unsigned octaves = (int)floor(float( m_uK/m_BPO))-1;
    for (unsigned octave = 0; octave <= octaves; octave++)
    {
	unsigned firstBin = octave*m_BPO;
	for (unsigned i = 0; i < m_BPO; i++)
	{
	    m_chromadata[i] += kabs( m_CQRe[ firstBin + i ], m_CQIm[ firstBin + i ]);
	}
    }

    MathUtilities::normalise(m_chromadata, m_BPO, m_normalise);

    return m_chromadata;
}


