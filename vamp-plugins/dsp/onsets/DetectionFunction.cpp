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
#include "maths/MathUtilities.h"
#include "DetectionFunction.h"
#include <cstring>

using namespace RBMixxxVamp;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DetectionFunction::DetectionFunction( DFConfig Config ) :
    m_DFType(Config.DFType)
   ,m_dataLength(Config.frameLength)
   ,m_halfLength(m_dataLength/2+1)
   ,m_stepSize(Config.stepSize)
   ,m_dbRise(Config.dbRise)
   ,m_whiten(Config.adaptiveWhitening)
   ,m_whitenRelaxCoeff(Config.whiteningRelaxCoeff)
   ,m_whitenFloor(Config.whiteningFloor)
   ,m_fft(RMFFT::Kaiser(m_dataLength,6.0f))

{
    if (m_whitenRelaxCoeff <= 0)
        m_whitenRelaxCoeff = 0.9997;
    if (m_whitenFloor <= 0)
        m_whitenFloor = 0.01;

    m_magPeaks = std::make_unique<float[]>( m_halfLength );
/*    m_magHistory = std::make_unique<float[]>( m_halfLength );
    m_phaseHistory = std::make_unique<float[]>( m_halfLength );
    m_phaseHistoryOld = std::make_unique<float[]>( m_halfLength );

    m_magnitude = std::make_unique<float[]>( m_halfLength );
    m_thetaAngle = std::make_unique<float[]>( m_halfLength );
    m_unwrapped = std::make_unique<float[]>( m_halfLength );

    m_windowed = std::make_unique<float[]>( m_dataLength );*/
}
DetectionFunction::~DetectionFunction() = default;

float DetectionFunction::processTimeDomain(const float*samples)
{
    m_spec.push_back();
    auto &spec = m_spec.back();
    m_fft.process(samples, spec, m_position + m_dataLength/2);
    m_position += m_stepSize;

/*    m_window.cut(samples, m_windowed.get());
    m_phaseVoc.processTimeDomain(m_windowed.get(),m_magnitude.get(), m_thetaAngle.get(), m_unwrapped.get());*/
    if (m_whiten)
        whiten();

    return runDF();
}

/*float DetectionFunction::processFrequencyDomain(const float *reals,
                                                 const float *imags)
{
    m_phaseVoc.processFrequencyDomain(
          reals
        , imags
        , &m_magnitude[0]
        , &m_thetaAngle[0]
        , &m_unwrapped[0]);
    if (m_whiten)
        whiten();
    return runDF();
}*/

void DetectionFunction::whiten()
{
    if(m_spec.empty())
        return;
    auto &spec = m_spec.back();
    auto mbeg = spec.mag_data(), mend = mbeg + m_halfLength;
    auto pbeg = m_magPeaks.get();//,pend = pbeg + m_halfLength;
    bs::transform(mbeg,mend, pbeg,pbeg,
        [&](auto m, auto peak) {
            auto npeak = bs::if_else(m < peak,m + (peak-m)*m_whitenRelaxCoeff, m);
            return bs::max(npeak, m_whitenFloor);
        });
    bs::transform(mbeg,mend, pbeg,mbeg,bs::divides);
    bs::transform(mbeg,mend, spec.M_data(),bs::log);
}
float DetectionFunction::runDF()
{
    auto retVal = 0.f;
    switch( DFType(m_DFType) )
    {
    case (DFType::NONE):
        break;
    case (DFType::HFC):
	retVal = HFC();
	break;

    case (DFType::SPECDIFF):
	retVal = specDiff();
	break;
    case (DFType::SPECDERIV):
	retVal = specDeriv();
	break;

    case (DFType::PHASEDEV):
        // Using the instantaneous phases here actually provides the
        // same results (for these calculations) as if we had used
        // unwrapped phases, but without the possible accumulation of
        // phase error over time
	retVal = phaseDev();
	break;
    case (DFType::D2PHI):
        // Using the instantaneous phases here actually provides the
        // same results (for these calculations) as if we had used
        // unwrapped phases, but without the possible accumulation of
        // phase error over time
	retVal = d2Phi();
	break;
    case (DFType::GROUPDELAY):
        // Using the instantaneous phases here actually provides the
        // same results (for these calculations) as if we had used
        // unwrapped phases, but without the possible accumulation of
        // phase error over time
	retVal = groupDelay();
	break;

    case (DFType::COMPLEXSD):
	retVal = complexSD();
	break;

    case (DFType::BROADBAND):
        retVal = broadband();
        break;
    default:
        std::cout << "unknown DetectionFunction value " << int(m_DFType) << std::endl;
        break;
    }

    return retVal;
}

float DetectionFunction::HFC()
{
    using reg = simd_reg<float>;
    constexpr auto w = int(simd_width<float>);
    auto len   = m_spec.back().coefficients();
    auto scale_factor = 2.0f / (bs::sqr(float(len)));
    auto &spec = m_spec.back();
    auto src   = spec.mag_data();
    auto i = 0;
    auto acc = reg(0.f);
    for ( ; i + w <= len; i += w)
        acc += reg(src + i) * bs::enumerate<reg>(i + 1);

    auto result = bs::sum(acc);
    for ( ; i <= len; ++i )
        result += src[i] * i;

    return result * scale_factor;
}

float DetectionFunction::specDiff()
{
    if(m_spec.size() < 2)
        return 0.0f;
    using reg = simd_reg<float>;
    constexpr auto w = int(simd_width<float>);
    auto &spec = m_spec.back();
    auto &pspec = m_spec[-2];
    auto len   = spec.coefficients();
    auto src   = spec.mag_data();
    auto psrc  = pspec.mag_data();
    auto i = 0;
    auto acc = reg(0.f);
    auto hwr = [](auto x){return x + bs::abs(x);};
    for ( ; i + w <= len; i += w)
        acc += hwr(bs::sqr_abs(reg(src + i)) - bs::sqr_abs(reg(psrc + i)));
    auto result = bs::sum(acc);
    for ( ; i <= len; ++i )
        result += hwr(bs::sqr_abs(*(src + i)) - bs::sqr_abs(*(psrc + i)));
    return result;
}
float DetectionFunction::phaseDev()
{
    if(m_spec.size() < 3)
        return 0.0f;
    using reg = simd_reg<float>;
    constexpr auto w = int(simd_width<float>);
    auto len    = m_spec[-1].coefficients();
    auto src    = m_spec[-1].Phi_data();
    auto psrc   = m_spec[-2].Phi_data();
    auto ppsrc  = m_spec[-3].Phi_data();
    auto i = 0;
    auto acc = reg(0.f);
    for ( ; i + w <= len; i += w) {
        auto tmpPhase = reg(src + i) - reg(2)*reg(psrc + i) + reg(ppsrc + i);
        acc += bs::abs(princarg(tmpPhase));
    }
    auto result = bs::sum(acc);
    for ( ; i <= len; ++i ) {
        auto tmpPhase = *(src + i) - 2*(*(psrc + i)) + *(ppsrc + i);
        result  += bs::abs(princarg(tmpPhase));
    }
    return result;
}
float DetectionFunction::groupDelay()
{
    if(m_spec.size() < 1)
        return 0.0f;
    using reg = simd_reg<float>;
    constexpr auto w = int(simd_width<float>);
    auto len    = m_spec[-1].coefficients();
    auto src    = m_spec[-1].dPhi_dw_data();
    auto mag    = m_spec[-1].mag_data();

    auto i = 0;
    auto acc = reg(0.f);
    auto wacc = reg(0.f);
    for ( ; i + w <= len; i += w) {
        auto _m = bs::sqrt(reg(mag + i));
        acc += reg(src + i) * _m;
        wacc += _m;
//        bs::abs(reg(src + i) - reg(psrc + i));
    }
    auto result = bs::sum(acc);
    auto wresult = bs::sum(wacc);
    for ( ; i <= len; ++i ) {
        auto _m = bs::sqrt(*(mag + i));
        result += *(src + i) * _m;
        wresult += _m;
    }
    if(wresult) {
        return ( len * 2 - (result / wresult));
    }else{
        return 0.0f;
    }
}

float DetectionFunction::specDeriv()
{
    if(m_spec.size() < 1)
        return 0.0f;
    using reg = simd_reg<float>;
    constexpr auto w = int(simd_width<float>);
    auto len    = m_spec[-1].coefficients();
    auto dmag   = m_spec[-1].dM_dt_data();
    auto mag    = m_spec[-1].mag_data();
    auto i = 0;
    auto acc = reg(0.f);
    auto hwr = [](auto x){return x + bs::abs(x);};
    for ( ; i + w <= len; i += w)
        acc += hwr(reg(dmag + i) * bs::sqrt(reg(mag + i)));
    auto result = bs::sum(acc);
    for ( ; i <= len; ++i )
        result += hwr(*(dmag + i) * bs::sqrt(*(mag + i)));
    return result;
}
float DetectionFunction::d2Phi()
{
    if(m_spec.size() < 2)
        return 0.0f;
    using reg = simd_reg<float>;
    constexpr auto w = int(simd_width<float>);
    auto len    = m_spec[-1].coefficients();
    auto src    = m_spec[-1].dPhi_dt_data();
    auto psrc   = m_spec[-2].dPhi_dt_data();
    auto i = 0;
    auto acc = reg(0.f);
    for ( ; i + w <= len; i += w)
        acc += bs::abs(reg(src + i) - reg(psrc + i));
    auto result = bs::sum(acc);
    for ( ; i <= len; ++i )
        result += bs::abs(*(src + i) - *(psrc + i));
    return result;
}

float DetectionFunction::complexSD()
{
    if(m_spec.size() < 3)
        return 0.0f;
    using reg = simd_reg<float>;
    constexpr auto w = int(simd_width<float>);
    auto len     = m_spec[-1].coefficients();
    auto src     = m_spec[-1].dPhi_dt_data();
    auto psrc    = m_spec[-2].dPhi_dt_data();
    auto hsrc    = m_spec[-1].mag_data();
    auto phsrc   = m_spec[-2].mag_data();
    auto i = 0;
    auto acc = reg(0.f);
    for( ; i + w < len; i+= w) {
        auto tmpPhase = reg(src + i) - reg(psrc + i);
        auto _i_r = bs::sincos(tmpPhase);
        auto hmag = reg(hsrc + i);
        acc += bs::hypot(reg(phsrc + i) - std::get<1>(_i_r) * hmag, std::get<0>(_i_r) * hmag);
    }
    auto result = bs::sum(acc);
    for( ; i < len; ++i) {
        auto tmpPhase = *(src + i) - *(psrc + i);
        auto _i_r = bs::sincos(tmpPhase);
        auto hmag = *(hsrc + i);
        result += bs::hypot(*(phsrc + i) - std::get<1>(_i_r) * hmag, std::get<0>(_i_r) * hmag);
    }
    return result;
}
float DetectionFunction::broadband()
{
    if(m_spec.size() < 2)
        return 0.0f;
    using reg = simd_reg<float>;
    constexpr auto w = int(simd_width<float>);
    auto len     = m_spec[-1].coefficients();
    auto src    = m_spec[-1].M_data();
    auto psrc   = m_spec[-2].M_data();
    auto i = 0;
    auto acc = reg(0.f);
    auto dbRise = reg(m_dbRise);
    for ( ; i + w <= len; i += w) {
        auto diff = bs::Ten<float>() * (reg(src + i) - reg(psrc + i))/bs::Log_10<float>();
        acc+= bs::if_one_else_zero(diff > dbRise);
    }
    auto result = bs::sum(acc);
    for ( ; i <= len; ++i ) {
        auto diff = bs::Ten<float>() * (*(src + i) - *(psrc + i))/bs::Log_10<float>();
        result += (diff > m_dbRise) ? 1.0f : 0.0f;
    }
    return result;
}
float * DetectionFunction::getSpectrumMagnitude()
{
    auto &spec = m_spec.back();
    return spec.mag_data();
}
