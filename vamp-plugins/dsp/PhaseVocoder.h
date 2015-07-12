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

#ifndef PHASEVOCODER_H
#define PHASEVOCODER_H

class FFT;

class PhaseVocoder  
{
public:
    PhaseVocoder( unsigned int size );
    virtual ~PhaseVocoder();

    void process( float* src, float* mag, float* theta);

protected:
    void getPhase(unsigned int size, float *theta, float *real, float *imag);
//    void coreFFT( unsigned int NumSamples, float *RealIn, float* ImagIn, float *RealOut, float *ImagOut);
    void getMagnitude( unsigned int size, float* mag, float* real, float* imag);
    void FFTShift( unsigned int size, float* src);

    unsigned int m_n;
    FFT*m_fft;
    float *m_imagOut;
    float *m_realOut;

};

#endif
