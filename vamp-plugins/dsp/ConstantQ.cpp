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
#include <memory>
#include <utility>
#include <algorithm>
#include "ConstantQ.h"
#include "FFT.h"


#ifdef NOT_DEFINED
// see note in CQprecalc

#include "CQprecalc.cpp"

static bool push_precalculated(int uk, int fftlength,
                               std::vector<size_t> &is,
                               std::vector<size_t> &js,
                               std::vector<double> &real,
                               std::vector<double> &imag)
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
static double nextpow2(double x) {
    double y = std::ceil(std::log(x)/std::log(2));
    return(y);
}

static double squaredModule(const double & xx, const double & yy) {
    return xx*xx + yy*yy;
}
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
void ConstantQ::sparsekernel(){
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

    auto hammingWindowRe = std::make_unique<double[]>(m_FFTLength);
    auto hammingWindowIm = std::make_unique<double[]> ( m_FFTLength );
    auto transfHammingWindowRe = std::make_unique<double[]> ( m_FFTLength );
    auto transfHammingWindowIm = std::make_unique<double[]> ( m_FFTLength );
    std::fill(&hammingWindowRe[0],&hammingWindowRe[m_FFTLength],0);
    std::fill(&hammingWindowIm[0],&hammingWindowIm[m_FFTLength],0);

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
    auto squareThreshold = m_CQThresh * m_CQThresh;
    FFT m_FFT(m_FFTLength);
    for (auto  k = m_uK; k--; ) {
        for (auto u=decltype(m_FFTLength){0}; u < m_FFTLength; u++) 
        {
            hammingWindowRe[u] = 0;
            hammingWindowIm[u] = 0;
        }
	// Computing a hamming window
	const auto hammingLength = static_cast<size_t>(std:: ceil( m_dQ * m_FS / ( m_FMin * std::pow(2,(static_cast<double>(k))/static_cast<double>(m_BPO)))));
        auto origin = m_FFTLength/2 - hammingLength/2;
	for (auto i=decltype(hammingLength){0}; i<hammingLength; i++) 
	{
	    const auto angle = 2*PI*m_dQ*i/hammingLength;
	    const auto real = std::cos(angle);
	    const auto imag = std::sin(angle);
	    const auto absol = hamming(hammingLength, i)/hammingLength;
	    hammingWindowRe[ origin + i ] = absol*real;
	    hammingWindowIm[ origin + i ] = absol*imag;
	}
        
        for (auto i = decltype(m_FFTLength){0}; i < m_FFTLength/2; ++i) {
            std::swap ( hammingWindowRe[i],hammingWindowRe[i+m_FFTLength/2]);
            std::swap ( hammingWindowIm[i],hammingWindowIm[i+m_FFTLength/2]);
        }
	//do fft of hammingWindow
	m_FFT.process( false, hammingWindowRe, hammingWindowIm, transfHammingWindowRe, transfHammingWindowIm );
	for (auto j=decltype(m_FFTLength){0}; j<( m_FFTLength ); j++) {
	    // perform thresholding
	    const auto squaredBin = squaredModule( transfHammingWindowRe[ j ], transfHammingWindowIm[ j ]);
	    if (squaredBin <= squareThreshold) continue;
	    // Insert non-zero position indexes, doubled because they are floats
	    sk->is.push_back(j);
	    sk->js.push_back(k);
	    // take conjugate, normalise and add to array sparkernel
	    sk->real.push_back( transfHammingWindowRe[ j ]/m_FFTLength);
	    sk->imag.push_back(-transfHammingWindowIm[ j ]/m_FFTLength);
	}

    }
/*
    using std::cout;
    using std::endl;

    cout.precision(28);

    int n = sk->is.size();
    int w = 8;
    cout << "static unsigned int sk_i_" << m_uK << "_" << m_FFTLength << "[" << n << "] = {" << endl;
    for (int i = 0; i < n; ++i) {
        if (i % w == 0) cout << "    ";
        cout << sk->is[i];
        if (i + 1 < n) cout << ", ";
        if (i % w == w-1) cout << endl;
    };
    if (n % w != 0) cout << endl;
    cout << "};" << endl;

    n = sk->js.size();
    cout << "static unsigned int sk_j_" << m_uK << "_" << m_FFTLength << "[" << n << "] = {" << endl;
    for (int i = 0; i < n; ++i) {
        if (i % w == 0) cout << "    ";
        cout << sk->js[i];
        if (i + 1 < n) cout << ", ";
        if (i % w == w-1) cout << endl;
    };
    if (n % w != 0) cout << endl;
    cout << "};" << endl;

    w = 2;
    n = sk->real.size();
    cout << "static double sk_real_" << m_uK << "_" << m_FFTLength << "[" << n << "] = {" << endl;
    for (int i = 0; i < n; ++i) {
        if (i % w == 0) cout << "    ";
        cout << sk->real[i];
        if (i + 1 < n) cout << ", ";
        if (i % w == w-1) cout << endl;
    };
    if (n % w != 0) cout << endl;
    cout << "};" << endl;

    n = sk->imag.size();
    cout << "static double sk_imag_" << m_uK << "_" << m_FFTLength << "[" << n << "] = {" << endl;
    for (int i = 0; i < n; ++i) {
        if (i % w == 0) cout << "    ";
        cout << sk->imag[i];
        if (i + 1 < n) cout << ", ";
        if (i % w == w-1) cout << endl;
    };
    if (n % w != 0) cout << endl;
    cout << "};" << endl;

    cout << "static void push_" << m_uK << "_" << m_FFTLength << "(vector<unsigned int> &is, vector<unsigned int> &js, vector<double> &real, vector<double> &imag)" << endl;
    cout << "{\n    is.reserve(" << n << ");\n";
    cout << "    js.reserve(" << n << ");\n";
    cout << "    real.reserve(" << n << ");\n";
    cout << "    imag.reserve(" << n << ");\n";
    cout << "    for (int i = 0; i < " << n << "; ++i) {" << endl;
    cout << "        is.push_back(sk_i_" << m_uK << "_" << m_FFTLength << "[i]);" << endl;
    cout << "        js.push_back(sk_j_" << m_uK << "_" << m_FFTLength << "[i]);" << endl;
    cout << "        real.push_back(sk_real_" << m_uK << "_" << m_FFTLength << "[i]);" << endl;
    cout << "        imag.push_back(sk_imag_" << m_uK << "_" << m_FFTLength << "[i]);" << endl;
    cout << "    }" << endl;
    cout << "}" << endl;
*/
//    std::cerr << "done\n -> is: " << sk->is.size() << ", js: " << sk->js.size() << ", reals: " << sk->real.size() << ", imags: " << sk->imag.size() << std::endl;
    m_sparseKernel = sk;
    return;
}

//-----------------------------------------------------------------------------
double* ConstantQ::process( const double* fftdata )
{
    if (!m_sparseKernel) {
        std::cerr << "ERROR: ConstantQ::process: Sparse kernel has not been initialised" << std::endl;
        return m_CQdata;
    }
    auto sk = m_sparseKernel;
    for (auto row=decltype(m_uK){0}; row<2*m_uK; row++) 
    {
	m_CQdata[ row ] = 0;
	m_CQdata[ row+1 ] = 0;
    }
    const auto fftbin = &(sk->is[0]);
    const auto cqbin  = &(sk->js[0]);
    const auto real   = &(sk->real[0]);
    const auto imag   = &(sk->imag[0]);
    const auto sparseCells = sk->real.size();
	
    for (auto i = decltype(sparseCells){0}; i<sparseCells; i++)
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
void ConstantQ::initialise( CQConfig Config ){
    m_FS = Config.FS;
    m_FMin = Config.min;		// min freq
    m_FMax = Config.max;		// max freq
    m_BPO = Config.BPO;		// bins per octave
    m_CQThresh = Config.CQThresh;// ConstantQ threshold for kernel generation

    m_dQ = 1/(std::pow(2,(1/(double)m_BPO))-1);	// Work out Q value for Filter bank
    m_uK =  std::ceil(m_BPO * std::log(m_FMax/m_FMin)/std::log(2.0));	// No. of constant Q bins

//    std::cerr << "ConstantQ::initialise: rate = " << m_FS << ", fmin = " << m_FMin << ", fmax = " << m_FMax << ", bpo = " << m_BPO << ", K = " << m_uK << ", Q = " << m_dQ << std::endl;

    // work out length of fft required for this constant Q Filter bank
    m_FFTLength = (int) std::pow(2, nextpow2(std::ceil( m_dQ*m_FS/m_FMin )));
    m_hop = m_FFTLength/8; // <------ hop size is window length divided by 32
//    std::cerr << "ConstantQ::initialise: -> fft length = " << m_FFTLength << ", hop = " << m_hop << std::endl;
    // allocate memory for cqdata
    m_CQdata = new double [2*m_uK];
}

void ConstantQ::deInitialise()
{
    delete [] m_CQdata;
    delete m_sparseKernel;
}

void ConstantQ::process(const double *FFTRe, const double* FFTIm,
                        double *CQRe, double *CQIm)
{
    if (!m_sparseKernel) {
        std::cerr << "ERROR: ConstantQ::process: Sparse kernel has not been initialised" << std::endl;
        return;
    }
    auto sk = m_sparseKernel;
    std::fill ( &CQRe[0],&CQRe[m_uK],0);
    std::fill ( &CQIm[0],&CQIm[m_uK],0);

    const auto fftbin = &(sk->is[0]);
    const auto cqbin  = &(sk->js[0]);
    const auto real   = &(sk->real[0]);
    const auto imag   = &(sk->imag[0]);
    const auto sparseCells = sk->real.size();
	
    for (auto i = decltype(sparseCells){0}; i<sparseCells; i++)
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
