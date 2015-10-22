/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file is based on Don Cross's public domain FFT implementation.
*/

#include "FFT.h"
#include "MathUtilities.h"
#include <cmath>
#include <iostream>

FFT::FFT(unsigned int n) :
    m_n(n),
    m_private(nullptr)
{
    if( !MathUtilities::isPowerOfTwo(m_n) )
    {
        std::cerr << "ERROR: FFT: Non-power-of-two FFT size "
                  << m_n << " not supported in this implementation"
                  << std::endl;
	return;
    }
}
FFT::~FFT() = default;

FFTReal::FFTReal(unsigned int n) :
    m_n(n),
    m_private ( std::make_unique<FFT>(m_n))
{
}
FFTReal::~FFTReal() = default;
void
FFTReal::process(bool inverse,
                 const float *realIn,
                 float *realOut, float *imagOut)
{
    m_private->process(inverse, realIn, 0, realOut, imagOut);
}
static unsigned int numberOfBitsNeeded(unsigned int p_nSamples)
{	
    if( p_nSamples < 2 ) return 0;
    for ( auto i=0; ; i++ )
    {
	if( p_nSamples & (1 << i) ) return i;
    }
}
static unsigned int reverseBits(unsigned int p_nIndex, unsigned int p_nBits)
{
    unsigned int i, rev;
    for(i=rev=0; i < p_nBits; i++)
    {
	rev = (rev << 1) | (p_nIndex & 1);
	p_nIndex >>= 1;
    }
    return rev;
}
void
FFT::process(bool p_bInverseTransform,
             const float *p_lpRealIn, const float *p_lpImagIn,
             float *p_lpRealOut, float *p_lpImagOut)
{
    if (!p_lpRealIn || !p_lpRealOut || !p_lpImagOut) return;
    unsigned int NumBits;
    unsigned int i, j, k, n;
    unsigned int BlockSize, BlockEnd;
    auto angle_numerator = static_cast<float>(2.0 * M_PI);
    auto tr = 0.f, ti = 0.f;
    if( !MathUtilities::isPowerOfTwo(m_n) )
    {
        std::cerr << "ERROR: FFT::process: Non-power-of-two FFT size "
                  << m_n << " not supported in this implementation"
                  << std::endl;
	return;
    }
    if( p_bInverseTransform ) angle_numerator = -angle_numerator;
    NumBits = numberOfBitsNeeded ( m_n );
    for( i=0; i < m_n; i++ )
    {
	j = reverseBits ( i, NumBits );
	p_lpRealOut[j] = p_lpRealIn[i];
	p_lpImagOut[j] = (p_lpImagIn == 0) ? 0.0 : p_lpImagIn[i];
    }
    BlockEnd = 1;
    for( BlockSize = 2; BlockSize <= m_n; BlockSize <<= 1 )
    {
	auto delta_angle = angle_numerator / (float)BlockSize;
	auto sm2 = -std::sin ( -2 * delta_angle );
	auto sm1 = -std::sin ( -delta_angle );
	auto cm2 = std::cos ( -2 * delta_angle );
	auto cm1 = std::cos ( -delta_angle );
	auto w = 2 * cm1;
	float ar[3], ai[3];
	for( i=0; i < m_n; i += BlockSize )
	{
	    ar[2] = cm2;
	    ar[1] = cm1;
	    ai[2] = sm2;
	    ai[1] = sm1;
	    for ( j=i, n=0; n < BlockEnd; j++, n++ )
	    {
		ar[0] = w*ar[1] - ar[2];
		ar[2] = ar[1];
		ar[1] = ar[0];
		ai[0] = w*ai[1] - ai[2];
		ai[2] = ai[1];
		ai[1] = ai[0];
		k = j + BlockEnd;
		tr = ar[0]*p_lpRealOut[k] - ai[0]*p_lpImagOut[k];
		ti = ar[0]*p_lpImagOut[k] + ai[0]*p_lpRealOut[k];
		p_lpRealOut[k] = p_lpRealOut[j] - tr;
		p_lpImagOut[k] = p_lpImagOut[j] - ti;
		p_lpRealOut[j] += tr;
		p_lpImagOut[j] += ti;
	    }
	}
	BlockEnd = BlockSize;
    }
    if( p_bInverseTransform )
    {
	auto denom = 1.f/m_n;
	for ( i=0; i < m_n; i++ )
	{
	    p_lpRealOut[i] *= denom;
	    p_lpImagOut[i] *= denom;
	}
    }
}
