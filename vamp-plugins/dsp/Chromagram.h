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

#ifndef CHROMAGRAM_H
#define CHROMAGRAM_H

#include <memory>
#include <utility>
#include "FFT.h"
#include "Window.h"
#include "ConstantQ.h"

struct ChromaConfig{
    size_t FS;
    double min;
    double max;
    size_t BPO;
    double CQThresh;
    MathUtilities::NormaliseType normalise;
};

class Chromagram 
{
public:	
    Chromagram( ChromaConfig Config );
    virtual ~Chromagram();
	
    double* process( const double *data ); // time domain
    double* process( const double *real, const double *imag ); // frequency domain
    void unityNormalise( double* src );

    // Complex arithmetic
    double kabs( double real, double imag );
	
    // Results
    size_t getK()         { return m_uK;}
    size_t getFrameSize() { return m_frameSize; }
    size_t getHopSize()   { return m_hopSize; }

private:
    int initialise( ChromaConfig Config );
    int deInitialise();

    std::unique_ptr<Window<double> >m_window    = nullptr;
    double         *m_windowbuf = nullptr;
	
    double* m_chromadata= nullptr;
    double       m_FMin = 0;
    double       m_FMax = 0;
    size_t       m_BPO  = 0;
    size_t       m_uK   = 0;

    MathUtilities::NormaliseType m_normalise;

    size_t       m_frameSize = 0;
    size_t       m_hopSize   = 0;

    std::unique_ptr<FFTReal>   m_FFT;
    std::unique_ptr<ConstantQ> m_ConstantQ;

    double* m_FFTRe    = nullptr;
    double* m_FFTIm    = nullptr;
    double* m_CQRe     = nullptr;
    double* m_CQIm     = nullptr;

    bool m_skGenerated = false;
};

#endif
