#ifndef ENGINEFILTERIIR_H
#define ENGINEFILTERIIR_H

#include <string.h>

#include "engine/engineobject.h"
#include "sampleutil.h"
#define MIXXX
#include <fidlib.h>

// set to 1 to print some analysis data using qDebug()
// It prints the resulting delay after 50 % of impulse have passed
// and the gain and phase shift at some sample frequencies
// You may also use the app fiview for analysis
#define IIR_ANALYSIS 0
#define FIDSPEC_LENGTH 40
#define MAX_SIZE 64

enum IIRPass {
    IIR_LP,
    IIR_BP,
    IIR_HP,
    IIR_LPMO,
    IIR_HPMO,
};


class EngineFilterIIRBase : public EngineObject{
  public:
    EngineFilterIIRBase(QObject*pParent=nullptr);
    virtual ~EngineFilterIIRBase();
    virtual void assumeSettled();
    virtual void pauseFilter();
    virtual void setCoefs(int SIZE, double sampleRate,const char* spec,
            double freq0, double freq1 = 0, int adj = 0) ;
    virtual void setCoefs2(int SIZE, double sampleRate, int n_coef1,
            const char* spec1, double freq01, double freq11, int adj1,
            const char* spec2, double freq02, double freq12, int adj2);
    virtual void initBuffers();
    virtual void processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize);

  protected:
    void pauseFilterInner();
    CSAMPLE_GAIN m_coef[MAX_SIZE + 1];
    // Old coefficients needed for ramping
    CSAMPLE_GAIN m_oldCoef[MAX_SIZE + 1];
    // Channel 1 state
    CSAMPLE m_buf1[MAX_SIZE];
    // Old channel 1 buffer needed for ramping
    CSAMPLE m_oldBuf1[MAX_SIZE];
    // Channel 2 state
    CSAMPLE m_buf2[MAX_SIZE];
    // Old channel 2 buffer needed for ramping
    CSAMPLE m_oldBuf2[MAX_SIZE];
    // Flag set to true if ramping needs to be done
    bool m_doRamping;
    // Flag set to true if old filter is invalid
    bool m_doStart;
    // Flag set to true if this is a chained filter
    bool m_startFromDry;

};


// length of the 3rd argument to fid_design_coef
template<unsigned int SIZE, enum IIRPass PASS>
class EngineFilterIIR : public EngineFilterIIRBase {
  public:
    EngineFilterIIR(QObject *pParent=nullptr)
            : EngineFilterIIRBase(pParent){
        memset(m_coef, 0, sizeof(m_coef));
        pauseFilter();
    }
    virtual ~EngineFilterIIR() {};
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize) {
        if (!m_doRamping) {
            for (int i = 0; i < iBufferSize; i += 2) {
                pOutput[i] = processSample(m_coef, m_buf1, pIn[i]);
                pOutput[i+1] = processSample(m_coef, m_buf2, pIn[i + 1]);
            }
        } else {
            CSAMPLE_GAIN cross_mix = 0.0f;
            CSAMPLE_GAIN cross_inc = 4.0f / static_cast<CSAMPLE_GAIN>(iBufferSize);
            for (int i = 0; i < iBufferSize; i += 2) {
                // Do a linear cross fade between the output of the old
                // Filter and the new filter.
                // The new filter is settled for Input = 0 and it sees
                // all frequencies of the rectangular start impulse.
                // Since the group delay, after which the start impulse
                // has passed is unknown here, we just what the half
                // iBufferSize until we use the samples of the new filter.
                // In one of the previous version we have faded the Input
                // of the new filter but it turns out that this produces
                // a gain drop due to the filter delay which is more
                // conspicuous than the settling noise.
                CSAMPLE old1;
                CSAMPLE old2;
                if (!m_doStart) {
                    // Process old filter, but only if we do not do a fresh start
                    old1 = processSample(m_oldCoef, m_oldBuf1, pIn[i + 0]);
                    old2 = processSample(m_oldCoef, m_oldBuf2, pIn[i + 1]);
                } else {
                    if (m_startFromDry) {
                        old1 = pIn[i + 0];
                        old2 = pIn[i + 1];
                    } else {
                        old1 = 0;
                        old2 = 0;
                    }
                }
                CSAMPLE new1 = processSample(m_coef, m_buf1, pIn[i]);
                CSAMPLE new2 = processSample(m_coef, m_buf2, pIn[i + 1]);
                if (i < iBufferSize / 2) {
                    pOutput[i + 0] = old1;
                    pOutput[i + 1] = old2;
                } else {
                    pOutput[i + 0] = new1 * cross_mix + old1 * (1.0f - cross_mix);
                    pOutput[i + 1] = new2  * cross_mix + old2 * (1.0f - cross_mix);
                    cross_mix += cross_inc;
                }
            }
            m_doRamping = false;
            m_doStart = false;
        }
    }
  protected:
    inline CSAMPLE processSample(CSAMPLE_GAIN * coef, CSAMPLE * buf, CSAMPLE val);
};
template<>
inline CSAMPLE EngineFilterIIR<2, IIR_LP>::processSample(CSAMPLE_GAIN * coef,CSAMPLE* buf,CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = tmp;
    iir -= coef[2] * buf[0]; fir += buf[0] + buf[0];
    fir += iir;
    buf[1] = iir; val = fir;
    return val;
}

template<>
inline CSAMPLE EngineFilterIIR<2, IIR_BP>::processSample(CSAMPLE_GAIN * coef,
                                                        CSAMPLE * buf,
                                                        CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = -tmp;
    iir -= coef[2] * buf[0];
    fir += iir;
    buf[1] = iir; val = fir;
    return val;
}

template<>
inline CSAMPLE EngineFilterIIR<2, IIR_HP>::processSample(CSAMPLE_GAIN* coef,
                                                        CSAMPLE * buf,
                                                        CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = tmp;
    iir -= coef[2] * buf[0]; fir += -buf[0] - buf[0];
    fir += iir;
    buf[1] = iir; val = fir;
    return val;
}

template<>
inline CSAMPLE EngineFilterIIR<4, IIR_LP>::processSample(CSAMPLE_GAIN* coef,
                                                        CSAMPLE * buf,
                                                        CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = tmp;
    iir -= coef[2] * buf[0]; fir += buf[0] + buf[0];
    fir += iir;
    tmp = buf[1]; buf[1] = iir; val = fir;
    iir = val;
    iir -= coef[3] * tmp; fir = tmp;
    iir -= coef[4] * buf[2]; fir += buf[2] + buf[2];
    fir += iir;
    buf[3] = iir; val = fir;
    return val;
}

template<>
inline CSAMPLE EngineFilterIIR<8, IIR_BP>::processSample(CSAMPLE_GAIN * coef,
                                                        CSAMPLE * buf,
                                                        CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3];
    buf[3] = buf[4]; buf[4] = buf[5]; buf[5] = buf[6]; buf[6] = buf[7];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = tmp;
    iir -= coef[2] * buf[0]; fir += -buf[0] - buf[0];
    fir += iir;
    tmp = buf[1]; buf[1] = iir; val= fir;
    iir = val;
    iir -= coef[3] * tmp; fir = tmp;
    iir -= coef[4] * buf[2]; fir += -buf[2] - buf[2];
    fir += iir;
    tmp = buf[3]; buf[3] = iir; val= fir;
    iir = val;
    iir -= coef[5] * tmp; fir = tmp;
    iir -= coef[6] * buf[4]; fir += buf[4] + buf[4];
    fir += iir;
    tmp = buf[5]; buf[5] = iir; val= fir;
    iir = val;
    iir -= coef[7] * tmp; fir = tmp;
    iir -= coef[8] * buf[6]; fir += buf[6] + buf[6];
    fir += iir;
    buf[7] = iir; val = fir;
    return val;
}

template<>
inline CSAMPLE EngineFilterIIR<4, IIR_HP>::processSample(CSAMPLE_GAIN * coef,
                                                        CSAMPLE * buf,
                                                        CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3];
    iir= val * coef[0];
    iir -= coef[1] * tmp; fir = tmp;
    iir -= coef[2] * buf[0]; fir += -buf[0] - buf[0];
    fir += iir;
    tmp = buf[1]; buf[1] = iir; val = fir;
    iir = val;
    iir -= coef[3] * tmp; fir = tmp;
    iir -= coef[4] * buf[2]; fir += -buf[2] - buf[2];
    fir += iir;
    buf[3] = iir; val = fir;
    return val;
}

template<>
inline CSAMPLE EngineFilterIIR<8, IIR_LP>::processSample(CSAMPLE_GAIN * coef,
                                                        CSAMPLE * buf,
                                                        CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3];
    buf[3] = buf[4]; buf[4] = buf[5]; buf[5] = buf[6]; buf[6] = buf[7];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = tmp;
    iir -= coef[2] * buf[0]; fir += buf[0] + buf[0];
    fir += iir;
    tmp = buf[1]; buf[1] = iir; val = fir;
    iir = val;
    iir -= coef[3] * tmp; fir = tmp;
    iir -= coef[4] * buf[2]; fir += buf[2] + buf[2];
    fir += iir;
    tmp = buf[3]; buf[3] = iir; val = fir;
    iir = val;
    iir -= coef[5] * tmp; fir = tmp;
    iir -= coef[6] * buf[4]; fir += buf[4] + buf[4];
    fir += iir;
    tmp = buf[5]; buf[5] = iir; val = fir;
    iir = val;
    iir -= coef[7] * tmp; fir = tmp;
    iir -= coef[8] * buf[6]; fir += buf[6] + buf[6];
    fir += iir;
    buf[7] = iir; val = fir;
    return val;
}

template<>
inline CSAMPLE EngineFilterIIR<16, IIR_BP>::processSample(CSAMPLE_GAIN* coef,
                                                         CSAMPLE * buf,
                                                         CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3];
    buf[3] = buf[4]; buf[4] = buf[5]; buf[5] = buf[6]; buf[6] = buf[7];
    buf[7] = buf[8]; buf[8] = buf[9]; buf[9] = buf[10]; buf[10] = buf[11];
    buf[11] = buf[12]; buf[12] = buf[13]; buf[13] = buf[14]; buf[14] = buf[15];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = tmp;
    iir -= coef[2] * buf[0]; fir += -buf[0] - buf[0];
    fir += iir;
    tmp = buf[1]; buf[1] = iir; val = fir;
    iir = val;
    iir -= coef[3] * tmp; fir = tmp;
    iir -= coef[4] * buf[2]; fir += -buf[2] - buf[2];
    fir += iir;
    tmp = buf[3]; buf[3] = iir; val = fir;
    iir = val;
    iir -= coef[5] * tmp; fir = tmp;
    iir -= coef[6] * buf[4]; fir += -buf[4] - buf[4];
    fir += iir;
    tmp = buf[5]; buf[5] = iir; val = fir;
    iir = val;
    iir -= coef[7] * tmp; fir = tmp;
    iir -= coef[8] * buf[6]; fir += -buf[6] - buf[6];
    fir += iir;
    tmp = buf[7]; buf[7]= iir; val= fir;
    iir = val;
    iir -= coef[9] * tmp; fir = tmp;
    iir -= coef[10] * buf[8]; fir += buf[8] + buf[8];
    fir += iir;
    tmp = buf[9]; buf[9] = iir; val = fir;
    iir = val;
    iir -= coef[11] * tmp; fir = tmp;
    iir -= coef[12] * buf[10]; fir += buf[10] + buf[10];
    fir += iir;
    tmp = buf[11]; buf[11] = iir; val = fir;
    iir = val;
    iir -= coef[13] * tmp; fir = tmp;
    iir -= coef[14] * buf[12]; fir += buf[12] + buf[12];
    fir += iir;
    tmp = buf[13]; buf[13] = iir; val = fir;
    iir = val;
    iir -= coef[15] * tmp; fir = tmp;
    iir -= coef[16] * buf[14]; fir += buf[14] + buf[14];
    fir += iir;
    buf[15] = iir; val = fir;
    return val;
}

template<>
inline CSAMPLE EngineFilterIIR<8, IIR_HP>::processSample(CSAMPLE_GAIN* coef,
                                                        CSAMPLE * buf,
                                                        CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3];
    buf[3] = buf[4]; buf[4] = buf[5]; buf[5] = buf[6]; buf[6] = buf[7];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = tmp;
    iir -= coef[2] * buf[0]; fir += -buf[0] - buf[0];
    fir += iir;
    tmp = buf[1]; buf[1] = iir; val = fir;
    iir = val;
    iir -= coef[3] * tmp; fir = tmp;
    iir -= coef[4] * buf[2]; fir += -buf[2] - buf[2];
    fir += iir;
    tmp = buf[3]; buf[3] = iir; val = fir;
    iir = val;
    iir -= coef[5] * tmp; fir = tmp;
    iir -= coef[6] * buf[4]; fir += -buf[4] - buf[4];
    fir += iir;
    tmp = buf[5]; buf[5] = iir; val = fir;
    iir = val;
    iir -= coef[7] * tmp; fir = tmp;
    iir -= coef[8] * buf[6]; fir += -buf[6] - buf[6];
    fir += iir;
    buf[7] = iir; val = fir;
    return val;
}

// IIR_LP and IIR_HP use the same processSample routine
template<>
inline CSAMPLE EngineFilterIIR<5, IIR_BP>::processSample(CSAMPLE_GAIN* coef,
                                                        CSAMPLE* buf,
                                                        CSAMPLE val) {
    CSAMPLE tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = coef[2] * tmp;
    iir -= coef[3] * buf[0]; fir += coef[4] * buf[0];
    fir += coef[5] * iir;
    buf[1] = iir; val = fir;
    return val;
}

template<>
inline CSAMPLE EngineFilterIIR<4, IIR_LPMO>::processSample(CSAMPLE_GAIN* coef,
                                                        CSAMPLE* buf,
                                                        CSAMPLE val) {
   CSAMPLE tmp, fir, iir;
   tmp= buf[0]; buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3];
   iir= val * coef[0];
   iir -= coef[1]*tmp; fir= tmp;
   fir += iir;
   tmp= buf[0]; buf[0]= iir; val= fir;
   iir= val;
   iir -= coef[2]*tmp; fir= tmp;
   fir += iir;
   tmp= buf[1]; buf[1]= iir; val= fir;
   iir= val;
   iir -= coef[3]*tmp; fir= tmp;
   fir += iir;
   tmp= buf[2]; buf[2]= iir; val= fir;
   iir= val;
   iir -= coef[4]*tmp; fir= tmp;
   fir += iir;
   buf[3]= iir; val= fir;
   return val;
}


template<>
inline CSAMPLE EngineFilterIIR<4, IIR_HPMO>::processSample(CSAMPLE_GAIN* coef,
                                                        CSAMPLE * buf,
                                                        CSAMPLE val) {
   CSAMPLE tmp, fir, iir;
   tmp= buf[0]; buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3];
   iir= val * coef[0];
   iir -= coef[1]*tmp; fir= -tmp;
   fir += iir;
   tmp= buf[0]; buf[0]= iir; val= fir;
   iir= val;
   iir -= coef[2]*tmp; fir= -tmp;
   fir += iir;
   tmp= buf[1]; buf[1]= iir; val= fir;
   iir= val;
   iir -= coef[3]*tmp; fir= -tmp;
   fir += iir;
   tmp= buf[2]; buf[2]= iir; val= fir;
   iir= val;
   iir -= coef[4]*tmp; fir= -tmp;
   fir += iir;
   buf[3]= iir; val= fir;
   return val;
}
#endif // ENGINEFILTERIIR_H
