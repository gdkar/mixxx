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

#include "maths/MathUtilities.h"
#include "maths/MathAliases.h"
#include "dsp/phasevocoder/PhaseVocoder.h"
#include "base/Window.h"

#define DF_HFC (1)
#define DF_SPECDIFF (2)
#define DF_PHASEDEV (3)
#define DF_COMPLEXSD (4)
#define DF_BROADBAND (5)

struct DFConfig{
    unsigned int stepSize; // DF step in samples
    unsigned int frameLength; // DF analysis window - usually 2*step. Must be even!
    int DFType; // type of detection function ( see defines )
    double dbRise; // only used for broadband df (and required for it)
    bool adaptiveWhitening; // perform adaptive whitening
    double whiteningRelaxCoeff; // if < 0, a sensible default will be used
    double whiteningFloor; // if < 0, a sensible default will be used
};

template<typename T>
class DetectionFunction  
{
public:
    DetectionFunction( const DFConfig &Config )
    {
        m_dataLength = Config.frameLength;
        m_halfLength = m_dataLength/2 + 1;

        m_DFType = Config.DFType;
        m_stepSize = Config.stepSize;
        m_dbRise = Config.dbRise;

        m_whiten = Config.adaptiveWhitening;
        m_whitenRelaxCoeff = Config.whiteningRelaxCoeff;
        m_whitenFloor = Config.whiteningFloor;
        if (m_whitenRelaxCoeff < 0)
            m_whitenRelaxCoeff = 0.9997;
        if (m_whitenFloor < 0)
            m_whitenFloor = 0.01;
        m_phaseVoc        = std::make_unique<PhaseVocoder<T> >(m_dataLength, m_stepSize);
        m_magHistory      = std::make_unique<T[]>(m_halfLength);
        m_phaseHistory    = std::make_unique<T[]>(m_halfLength);
        m_phaseHistoryOld = std::make_unique<T[]>(m_halfLength);
        m_magPeaks        = std::make_unique<T[]>(m_halfLength);
        m_magnitude       = std::make_unique<T[]>(m_halfLength);
        m_thetaAngle      = std::make_unique<T[]>(m_halfLength);
        m_unwrapped       = std::make_unique<T[]>(m_halfLength);
        m_window          = std::make_unique<Window<T> >(HanningWindow,m_dataLength);
        m_windowed        = std::make_unique<T[]>(m_dataLength);
    }
    virtual ~DetectionFunction() = default;
    /* * Process a single time-domain frame of audio, provided as
     * frameLength samples.
     */
    template<typename U>
    U processTimeDomain(const U* samples)
    {
        m_window->cut(samples,m_windowed.get());
        m_phaseVoc->processTimeDomain(m_windowed.get(),m_magnitude.get(),m_thetaAngle.get(),m_unwrapped.get());
        if(m_whiten)
            whiten();
        return runDF();
    }
    /**
     * Process a single frequency-domain frame, provided as
     * frameLength/2+1 real and imaginary component values.
     */
    template<typename U>
    T  processFrequencyDomain(const U* reals, const U* imags)
    {
        m_phaseVoc->processFrequencyDomain(reals,imags,m_magnitude.get(), m_thetaAngle.get(), m_unwrapped.get());
        if (m_whiten)
            whiten();
        return runDF();
    }
    T* getSpectrumMagnitude() const
    {
        return m_magnitude.get();
    }
    void whiten()
    {
        for (auto i = 0u; i < m_halfLength; ++i) {
            auto m = m_magnitude[i];
            if (m < m_magPeaks[i])
                m += (m_magPeaks[i] - m) * m_whitenRelaxCoeff;
            m = std::max(m,m_whitenFloor);
            m_magPeaks[i] = m;
            m_magnitude[i] /= m;
        }
    }
    T  runDF()
    {
        switch( m_DFType )
        {
        case DF_HFC:
            return  HFC( m_halfLength, m_magnitude.get());
        case DF_SPECDIFF:
            return specDiff( m_halfLength, m_magnitude.get());
        case DF_PHASEDEV:
            // Using the instantaneous phases here actually provides the
            // same results (for these calculations) as if we had used
            // unwrapped phases, but without the possible accumulation of
            // phase error over time
            return phaseDev( m_halfLength, m_thetaAngle.get());
        case DF_COMPLEXSD:
            return complexSD( m_halfLength, m_magnitude.get(), m_thetaAngle.get());
        case DF_BROADBAND:
            return broadband( m_halfLength, m_magnitude.get());
        default:
            return T{0};
        }
    }
    T  HFC( unsigned int length, T* src)
    {
        auto val = T{0};
        for( auto i = 0u; i < length; i++)
            val += src[ i ] * ( i + 1);
        return val;
    }
    T  specDiff(unsigned int length, T *src)
    {
        T val = 0.0;
        for( auto i = 0u; i < length; i++)
        {
            auto temp = std::abs( (src[ i ] * src[ i ]) - (m_magHistory[ i ] * m_magHistory[ i ]) );
            auto diff= std::sqrt(temp);
            // (See note in phaseDev below.)
            val += diff;
            m_magHistory[ i ] = src[ i ];
        }
        return val;
}
    T  phaseDev(unsigned int length, T *srcPhase)
    {
        auto val = T{0};
        for( auto i = 0u; i < length; i++)
        {
            auto tmpPhase = (srcPhase[ i ]- 2*m_phaseHistory[ i ]+m_phaseHistoryOld[ i ]);
            auto dev = MathUtilities::princarg( tmpPhase );
            // A previous version of this code only counted the value here
            // if the magnitude exceeded 0.1.  My impression is that
            // doesn't greatly improve the results for "loud" music (so
            // long as the peak picker is reasonably sophisticated), but
            // does significantly damage its ability to work with quieter
            // music, so I'm removing it and counting the result always.
            // Same goes for the spectral difference measure above.
                    
            auto tmpVal  = std::abs(dev);
            val += tmpVal ;
            m_phaseHistoryOld[ i ] = m_phaseHistory[ i ] ;
            m_phaseHistory[ i ] = srcPhase[ i ];
        }
        return val;
    }
    T  complexSD(unsigned int length, T *srcMagnitude, T *srcPhase)
    {
        auto val = T{0};
        ComplexData meas = ComplexData( 0, 0 );
        ComplexData j = ComplexData( 0, 1 );

        for( auto i = 0u; i < length; i++)
        {
            auto tmpPhase = (srcPhase[ i ]- 2*m_phaseHistory[ i ]+m_phaseHistoryOld[ i ]);
            auto dev= MathUtilities::princarg( tmpPhase );
                    
            meas = m_magHistory[i] - ( srcMagnitude[ i ] * exp( j * dev) );
            auto tmpReal = real( meas );
            auto tmpImag = imag( meas );

            val += std::sqrt( (tmpReal * tmpReal) + (tmpImag * tmpImag) );
                    
            m_phaseHistoryOld[ i ] = m_phaseHistory[ i ] ;
            m_phaseHistory[ i ] = srcPhase[ i ];
            m_magHistory[ i ] = srcMagnitude[ i ];
        }
        return val;
    }
    T broadband(unsigned int length, T *src)
    {
        auto val = T{0};
        for (auto i = 0u; i < length; ++i) {
            auto sqrmag = src[i] * src[i];
            if (m_magHistory[i] > 0.0) {
                auto diff = T{10.0} * std::log10(sqrmag / m_magHistory[i]);
                if (diff > m_dbRise)
                    val = val + 1;
            }
            m_magHistory[i] = sqrmag;
        }
        return val;
    }
private:

    int m_DFType;
    unsigned int m_dataLength;
    unsigned int m_halfLength;
    unsigned int m_stepSize;
    T m_dbRise;
    bool m_whiten;
    T m_whitenRelaxCoeff;
    T m_whitenFloor;

    std::unique_ptr<T[]> m_magHistory;
    std::unique_ptr<T[]> m_phaseHistory;
    std::unique_ptr<T[]> m_phaseHistoryOld;
    std::unique_ptr<T[]> m_magPeaks;

    std::unique_ptr<T[]> m_windowed; // Array for windowed analysis frame
    std::unique_ptr<T[]> m_magnitude; // Magnitude of analysis frame ( frequency domain )
    std::unique_ptr<T[]> m_thetaAngle;// Phase of analysis frame ( frequency domain )
    std::unique_ptr<T[]> m_unwrapped; // Unwrapped phase of analysis frame

    std::unique_ptr<Window<T> >    m_window;
    std::unique_ptr<PhaseVocoder<T> > m_phaseVoc;	// Phase Vocoder
};
