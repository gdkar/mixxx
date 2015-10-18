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

_Pragma("once")
#include <memory>
#include <utility>
#include "FFT.h"
#include "Window.h"
#include "ConstantQ.h"

struct ChromaConfig{
    size_t FS;
    float min;
    float max;
    size_t BPO;
    float CQThresh;
    MathUtilities::NormaliseType normalise;
};

class Chromagram 
{
public:	
    Chromagram( ChromaConfig Config );
    virtual ~Chromagram();
	
    float * process( const float *data ); // time domain
    float * process( const float *real, const float *imag ); // frequency domain
    void unityNormalise( float * src );

    // Complex arithmetic
    float kabs( float real, float imag );
	
    // Results
    size_t getK()         { return m_uK;}
    size_t getFrameSize() { return m_frameSize; }
    size_t getHopSize()   { return m_hopSize; }

private:
    int initialise( ChromaConfig Config );
    int deInitialise();

    std::unique_ptr<Window<float > >m_window    = nullptr;
    float *m_windowbuf = nullptr;
	
    float * m_chromadata= nullptr;
    float m_FMin = 0;
    float m_FMax = 0;
    size_t       m_BPO  = 0;
    size_t       m_uK   = 0;

    MathUtilities::NormaliseType m_normalise;

    size_t       m_frameSize = 0;
    size_t       m_hopSize   = 0;

    std::unique_ptr<FFTReal>   m_FFT;
    std::unique_ptr<ConstantQ> m_ConstantQ;

    float* m_FFTRe    = nullptr;
    float* m_FFTIm    = nullptr;
    float* m_CQRe     = nullptr;
    float* m_CQIm     = nullptr;

    bool m_skGenerated = false;
};
