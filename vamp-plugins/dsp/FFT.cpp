/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file is based on Don Cross's public domain FFT implementation.
*/
#include "FFT.h"
#include "MathUtilities.h"
#include <cmath>
#include <cstring>
// M_PI needs to be difined for Windows builds
#include <iostream>
#include <alloca.h>
FFT::FFT(int n) :
    m_n(n){
    auto _ir = (float*)alloca(n*sizeof(float));
    auto _ii = (float*)alloca(n*sizeof(float));
    auto _or = (float*)alloca(n*sizeof(float));
    auto _oi = (float*)alloca(n*sizeof(float));

    fftw_iodim dims[] = { {.n = n, .is = 1, .os = 1}};
    m_plan  = fftwf_plan_guru_split_dft(1,dims,0,nullptr,_ir,_ii,_or,_oi,FFTW_MEASURE);
}
FFT::~FFT(){fftwf_destroy_plan(m_plan);}
void
FFT::process(bool _inverse,float *p_lpRealIn, float *p_lpImagIn,float *p_lpRealOut, float *p_lpImagOut){
    if(_inverse){
        float *tmp = p_lpRealIn;
        p_lpRealIn = p_lpImagIn;
        p_lpImagIn = tmp;
        tmp = p_lpRealOut;
        p_lpRealOut=p_lpImagOut;
        p_lpImagOut=tmp;
    }
    if(!p_lpRealIn){ p_lpRealIn = (float*)alloca(m_n*sizeof(float));std::memset(p_lpRealIn,0,m_n*sizeof(float));}
    if(!p_lpImagIn){ p_lpImagIn = (float*)alloca(m_n*sizeof(float));std::memset(p_lpImagIn,0,m_n*sizeof(float));}
    if(!p_lpRealOut) p_lpRealOut = (float*)alloca(m_n*sizeof(float));
    if(!p_lpImagOut) p_lpImagOut = (float*)alloca(m_n*sizeof(float));
    fftwf_execute_split_dft(m_plan, p_lpRealIn, p_lpImagIn, p_lpRealOut, p_lpImagOut);
}

