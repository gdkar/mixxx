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
#include <vector>
#include <memory>
#include "MathAliases.h"
#include "MathUtilities.h"

struct CQConfig{
    size_t FS;   // samplerate
    float min;        // minimum frequency
    float max;        // maximum frequency
    size_t BPO;  // bins per octave
    float CQThresh;   // threshold
};

class ConstantQ {
	
//public functions incl. sparsekernel so can keep out of loop in main
public:
    void process( const float * FFTRe, const float * FFTIm,
                  float * CQRe, float * CQIm );

    ConstantQ( CQConfig Config );
    virtual ~ConstantQ();

    float * process( const float* FFTData );

    void sparsekernel();

    float hamming(int len, int n) {
	return  static_cast<float>(0.54f - 0.46f*std::cos(2*M_PI*n/len));
    }
	
    size_t getnumwin() { return m_numWin;}
    float getQ() { return m_dQ;}
    size_t getK() {return m_uK ;}
    size_t getfftlength() { return m_FFTLength;}
    size_t gethop() { return m_hop;}

private:
    void initialise( CQConfig Config );
    void deInitialise();
	
    float * m_CQdata;
    size_t m_FS;
    float m_FMin;
    float m_FMax;
    float m_dQ;
    float m_CQThresh;
    size_t m_numWin;
    size_t m_hop;
    size_t m_BPO;
    size_t m_FFTLength;
    size_t m_uK;

    struct SparseKernel {
        std::vector<size_t> is;
        std::vector<size_t> js;
        std::vector<float > imag;
        std::vector<float> real;
    };
    SparseKernel *m_sparseKernel;
};
