/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/*
    QM DSP Library
    Centre for Digital Music, Queen Mary, University of London.
*/
#ifndef FFT_H
#define FFT_H

#include <fftw3.h>

class FFT{
public:
    FFT(int nsamples);
    ~FFT();
    void process(bool inverse,float *realIn, float *imagIn,float *realOut, float *imagOut);
private:
    int m_n;
    fftwf_plan m_plan;
};
#endif
