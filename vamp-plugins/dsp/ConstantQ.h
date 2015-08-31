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

#ifndef CONSTANTQ_H
#define CONSTANTQ_H

#include <vector>
#include <memory>
#include "MathAliases.h"
#include "MathUtilities.h"

struct CQConfig{
    size_t FS;   // samplerate
    double min;        // minimum frequency
    double max;        // maximum frequency
    size_t BPO;  // bins per octave
    double CQThresh;   // threshold
};

class ConstantQ {
	
//public functions incl. sparsekernel so can keep out of loop in main
public:
    void process( const double* FFTRe, const double* FFTIm,
                  double* CQRe, double* CQIm );

    ConstantQ( CQConfig Config );
    virtual ~ConstantQ();

    double* process( const double* FFTData );

    void sparsekernel();

    double hamming(int len, int n) {
	double out = 0.54 - 0.46*cos(2*PI*n/len);
	return(out);
    }
	
    size_t getnumwin() { return m_numWin;}
    double getQ() { return m_dQ;}
    size_t getK() {return m_uK ;}
    size_t getfftlength() { return m_FFTLength;}
    size_t gethop() { return m_hop;}

private:
    void initialise( CQConfig Config );
    void deInitialise();
	
    double* m_CQdata;
    size_t m_FS;
    double m_FMin;
    double m_FMax;
    double m_dQ;
    double m_CQThresh;
    size_t m_numWin;
    size_t m_hop;
    size_t m_BPO;
    size_t m_FFTLength;
    size_t m_uK;

    struct SparseKernel {
        std::vector<size_t> is;
        std::vector<size_t> js;
        std::vector<double> imag;
        std::vector<double> real;
    };
    SparseKernel *m_sparseKernel;
};


#endif//CONSTANTQ_H

