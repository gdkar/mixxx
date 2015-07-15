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

#include "ConstantQ.h"
#include "FFT.h"

#include <iostream>

#ifdef NOT_DEFINED
// see note in CQprecalc

#include "CQprecalc.cpp"

static bool push_precalculated(int uk, int fftlength,
                               std::vector<unsigned> &is,
                               std::vector<unsigned> &js,
                               std::vector<float> &real,
                               std::vector<float> &imag)
{
    if (uk == 76 && fftlength == 16384) {
        push_76_16384(is, js, real, imag);
        return true;
    }
    if (uk == 144 && fftlength == 4096) {
        push_144_4096(is, js, real, imag);
        return true;
    }
    if (uk == 65 && fftlength == 2048) {
        push_65_2048(is, js, real, imag);
        return true;
    }
    if (uk == 84 && fftlength == 65536) {
        push_84_65536(is, js, real, imag);
        return true;
    }
    return false;
}
#endif
//---------------------------------------------------------------------------
// nextpow2 returns the smallest integer n such that 2^n >= x.
static float nextpow2(float x) {
    float y = std::ceil(std::log(x)/std::log(2.0f));
    return(y);
}
template<typename T>
static T squaredModule(const T& xx, const T& yy) {return xx*xx + yy*yy;}

//----------------------------------------------------------------------------

ConstantQ::ConstantQ( CQConfig Config ) :
    m_sparseKernel(0)
{
    initialise( Config );
}

ConstantQ::~ConstantQ()
{
    deInitialise();
}

//----------------------------------------------------------------------------
void ConstantQ::sparsekernel()
{
//    std::cerr << "ConstantQ: initialising sparse kernel, uK = " << m_uK << ", FFTLength = " << m_FFTLength << "...";
    SparseKernel *sk = new SparseKernel();
#ifdef NOT_DEFINED
    if (push_precalculated(m_uK, m_FFTLength, sk->is, sk->js, sk->real, sk->imag)) {
//        std::cerr << "using precalculated kernel" << std::endl;
        m_sparseKernel = sk;
        return;
    }
#endif
    //generates spectral kernel matrix (upside down?)
    // initialise temporal kernel with zeros, twice length to deal w. complex numbers
    float* hammingWindowRe = new float [ m_FFTLength ];
    float* hammingWindowIm = new float [ m_FFTLength ];
    float* transfHammingWindowRe = new float [ m_FFTLength ];
    float* transfHammingWindowIm = new float [ m_FFTLength ];
    for (unsigned u=0; u < m_FFTLength; u++) 
    {
	hammingWindowRe[u] = 0;
	hammingWindowIm[u] = 0;
    }
    // Here, fftleng*2 is a guess of the number of sparse cells in the matrix
    // The matrix K x fftlength but the non-zero cells are an antialiased
    // square root function. So mostly is a line, with some grey point.
    sk->is.reserve( m_FFTLength*2 );
    sk->js.reserve( m_FFTLength*2 );
    sk->real.reserve( m_FFTLength*2 );
    sk->imag.reserve( m_FFTLength*2 );
    // for each bin value K, calculate temporal kernel, take its fft to
    //calculate the spectral kernel then threshold it to make it sparse and 
    //add it to the sparse kernels matrix
    float squareThreshold = m_CQThresh * m_CQThresh;
    FFT m_FFT(m_FFTLength);
    for (unsigned k = m_uK; k--; ) {
        for (unsigned u=0; u < m_FFTLength; u++) {
            hammingWindowRe[u] = 0;
            hammingWindowIm[u] = 0;
        }
	// Computing a hamming window
	auto hammingLength = (int) std::ceil( m_dQ * m_FS / ( m_FMin * std::pow(2,((float)(k))/(float)m_BPO)));
        auto origin = m_FFTLength/2 - hammingLength/2;
	for (auto i=0; i<hammingLength; i++) 
	{
	    const float angle = 2*M_PI*m_dQ*i/hammingLength;
	    const float real = cos(angle);
	    const float imag = sin(angle);
	    const float absol = hamming(hammingLength, i)/hammingLength;
	    hammingWindowRe[ origin + i ] = absol*real;
	    hammingWindowIm[ origin + i ] = absol*imag;
	}

        for (unsigned i = 0; i < m_FFTLength/2; ++i) {
            float temp = hammingWindowRe[i];
            hammingWindowRe[i] = hammingWindowRe[i + m_FFTLength/2];
            hammingWindowRe[i + m_FFTLength/2] = temp;
            temp = hammingWindowIm[i];
            hammingWindowIm[i] = hammingWindowIm[i + m_FFTLength/2];
            hammingWindowIm[i + m_FFTLength/2] = temp;
        }
	//do fft of hammingWindow
	m_FFT.process( 0, hammingWindowRe, hammingWindowIm, transfHammingWindowRe, transfHammingWindowIm );
	for (auto j=0; j<( m_FFTLength ); j++) 
	{
	    // perform thresholding
	    const float squaredBin = squaredModule( transfHammingWindowRe[ j ], transfHammingWindowIm[ j ]);
	    if (squaredBin <= squareThreshold) continue;
	    // Insert non-zero position indexes, floatd because they are floats
	    sk->is.push_back(j);
	    sk->js.push_back(k);
	    // take conjugate, normalise and add to array sparkernel
	    sk->real.push_back( transfHammingWindowRe[ j ]/m_FFTLength);
	    sk->imag.push_back(-transfHammingWindowIm[ j ]/m_FFTLength);
	}

    }
    delete [] hammingWindowRe;
    delete [] hammingWindowIm;
    delete [] transfHammingWindowRe;
    delete [] transfHammingWindowIm;
    
    m_sparseKernel = sk;
    return;
}
//-----------------------------------------------------------------------------
float* ConstantQ::process( const float* fftdata )
{
    if (!m_sparseKernel) {
        std::cerr << "ERROR: ConstantQ::process: Sparse kernel has not been initialised" << std::endl;
        return m_CQdata;
    }
    SparseKernel *sk = m_sparseKernel;
    for (unsigned row=0; row<2*m_uK; row++) 
    {
	m_CQdata[ row ] = 0;
	m_CQdata[ row+1 ] = 0;
    }
    const auto    *fftbin = &(sk->is[0]);
    const auto    *cqbin  = &(sk->js[0]);
    const auto    *real   = &(sk->real[0]);
    const auto    *imag   = &(sk->imag[0]);
    const auto sparseCells = sk->real.size();
	
    for (auto i = 0; i<sparseCells; i++)
    {
	const auto row = cqbin[i];
	const auto col = fftbin[i];
	const auto & r1  = real[i];
	const auto & i1  = imag[i];
	const auto & r2  = fftdata[ (2*m_FFTLength) - 2*col - 2 ];
	const auto & i2  = fftdata[ (2*m_FFTLength) - 2*col - 2 + 1 ];
	// add the multiplication
	m_CQdata[ 2*row  ] += (r1*r2 - i1*i2);
	m_CQdata[ 2*row+1] += (r1*i2 + i1*r2);
    }
    return m_CQdata;
}


void ConstantQ::initialise( CQConfig Config )
{
    m_FS = Config.FS;
    m_FMin = Config.min;		// min freq
    m_FMax = Config.max;		// max freq
    m_BPO = Config.BPO;		// bins per octave
    m_CQThresh = Config.CQThresh;// ConstantQ threshold for kernel generation
    m_dQ = 1/(std::pow(2,(1/(float)m_BPO))-1);	// Work out Q value for Filter bank
    m_uK = (unsigned int) std::ceil(m_BPO * std::log(m_FMax/m_FMin)/std::log(2.0f));	// No. of constant Q bins
    // work out length of fft required for this constant Q Filter bank
    m_FFTLength = (int) std::pow(2, nextpow2(std::ceil( m_dQ*m_FS/m_FMin )));
    m_hop = m_FFTLength/8; // <------ hop size is window length divided by 32
    // allocate memory for cqdata
    m_CQdata = new float [2*m_uK];
}
void ConstantQ::deInitialise()
{
    delete [] m_CQdata;
    delete m_sparseKernel;
}
void ConstantQ::process(const float *FFTRe, const float* FFTIm, float *CQRe, float *CQIm)
{
    if (!m_sparseKernel) {
        std::cerr << "ERROR: ConstantQ::process: Sparse kernel has not been initialised" << std::endl;
        return;
    }
    SparseKernel *sk = m_sparseKernel;
    for (auto row=0; row<m_uK; row++) 
    {
	CQRe[ row ] = 0;
	CQIm[ row ] = 0;
    }
    const auto *fftbin = &(sk->is[0]);
    const auto *cqbin  = &(sk->js[0]);
    const auto *real   = &(sk->real[0]);
    const auto *imag   = &(sk->imag[0]);
    const auto  sparseCells = sk->real.size();
    for (auto i = 0; i<sparseCells; i++)
    {
	const auto row = cqbin[i];
	const auto col = fftbin[i];
	const auto & r1  = real[i];
	const auto & i1  = imag[i];
	const auto & r2  = FFTRe[ m_FFTLength - col - 1 ];
	const auto & i2  = FFTIm[ m_FFTLength - col - 1 ];
	// add the multiplication
	CQRe[ row ] += (r1*r2 - i1*i2);
	CQIm[ row ] += (r1*i2 + i1*r2);
    }
}
