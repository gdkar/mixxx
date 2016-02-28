_Pragma("once")
#include <cstdio>
#include <fidlib.h>

#include "engine/engineobject.h"
#include "util/sample.h"

enum IIRPass {
    IIR_LP,
    IIR_BP,
    IIR_HP,
};


class EngineFilterIIRBase : public EngineObjectConstIn {
  public:
  Fid m_fid;
  virtual ~EngineFilterIIRBase() = default;
  virtual void assumeSettled() = 0;
  virtual void pauseFilter() = 0;
    virtual void processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize) = 0;
  virtual void initBuffers() = 0;
  virtual void setCoefs(const char*, double, double, double, int) = 0;
  virtual void setCoefs2(double sampleRate, int n_coef1,
            const char* spec1, double freq01, double freq11, int adj1,
            const char* spec2, double freq02, double freq12, int adj2) = 0;
  virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,
                         const int iBufferSize) = 0;
};

#define FIDSPEC_LENGTH 40

template<unsigned int SIZE, IIRPass PASS>
class EngineFilterIIR : public EngineFilterIIRBase {
  public:
    EngineFilterIIR()
            : m_doRamping(false),
              m_doStart(false),
              m_startFromDry(false) {
        memset(m_coef, 0, sizeof(m_coef));
        pauseFilter();
    }
    virtual ~EngineFilterIIR()= default;
    // this can be called continuously for Filters that have own ramping
    // or need no fade when disabling
    virtual void pauseFilter() {
        if (!m_doStart)
            pauseFilterInner();
    }
    // this is can be used instead off a final process() call before pause
    // It fades to dry or 0 according to the m_startFromDry parameter
    // it is an alternative for using pauseFillter() calls
    virtual void processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize) {
        process(pIn, pOutput, iBufferSize);
        SampleUtil::copyWithRampingGain(pOutput,pOutput,1,0,iBufferSize);
        if(m_startFromDry)
            SampleUtil::addWithRampingGain(pOutput,pIn, 0, 1 , iBufferSize);
        pauseFilterInner();
    }
    virtual void initBuffers() {
        // Copy the current buffers into the old buffers
        memcpy(m_oldBuf1, m_buf1, sizeof(m_buf1));
        memcpy(m_oldBuf2, m_buf2, sizeof(m_buf2));
        // Set the current buffers to 0
        memset(m_buf1, 0, sizeof(m_buf1));
        memset(m_buf2, 0, sizeof(m_buf2));
        m_doRamping = true;
    }
    virtual void setCoefs(const char* spec, double sampleRate,double freq0, double freq1 = 0, int adj = 0) {
        char spec_d[FIDSPEC_LENGTH];
        if (strlen(spec) < sizeof(spec_d)) {
            // Copy to dynamic-ish memory to prevent fidlib API breakage.
            strcpy(spec_d, spec);
            // Copy the old coefficients into m_oldCoef
            memcpy(m_oldCoef, m_coef, sizeof(m_coef));
            m_coef[0] = m_fid.design_coef(m_coef + 1, SIZE,spec_d, sampleRate, freq0, freq1, adj);
            initBuffers();

        }
    }
    virtual void setCoefs2(double sampleRate, int n_coef1,
            const char* spec1, double freq01, double freq11, int adj1,
            const char* spec2, double freq02, double freq12, int adj2) {
        char spec1_d[FIDSPEC_LENGTH];
        char spec2_d[FIDSPEC_LENGTH];
        if (strlen(spec1) < sizeof(spec1_d) &&
                strlen(spec2) < sizeof(spec2_d)) {
            // Copy to dynamic-ish memory to prevent fidlib API breakage.
            strcpy(spec1_d, spec1);
            strcpy(spec2_d, spec2);

            // Copy the old coefficients into m_oldCoef
            memcpy(m_oldCoef, m_coef, sizeof(m_coef));
            m_coef[0] = m_fid.design_coef(m_coef + 1, n_coef1,
                    spec1, sampleRate, freq01, freq11, adj1) *
                        m_fid.design_coef(m_coef + 1 + n_coef1, SIZE - n_coef1,
                    spec2, sampleRate, freq02, freq12, adj2);

            initBuffers();
        }
    }
    virtual void assumeSettled() {
        m_doRamping = false;
        m_doStart = false;
    }
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,
                         const int iBufferSize) {
        if (!m_doRamping) {
            for (int i = 0; i < iBufferSize; i += 2) {
                pOutput[i] = processSample(m_coef, m_buf1, pIn[i]);
                pOutput[i+1] = processSample(m_coef, m_buf2, pIn[i + 1]);
            }
        } else {
            double cross_mix = 0.0;
            double cross_inc = 4.0 / static_cast<double>(iBufferSize);
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
                double old1;
                double old2;
                if (!m_doStart) {
                    // Process old filter, but only if we do not do a fresh start
                    old1 = processSample(m_oldCoef, m_oldBuf1, pIn[i]);
                    old2 = processSample(m_oldCoef, m_oldBuf2, pIn[i + 1]);
                } else {
                    if (m_startFromDry) {
                        old1 = pIn[i];
                        old2 = pIn[i + 1];
                    } else {
                        old1 = 0;
                        old2 = 0;
                    }
                }
                double new1 = processSample(m_coef, m_buf1, pIn[i]);
                double new2 = processSample(m_coef, m_buf2, pIn[i + 1]);

                if (i < iBufferSize / 2) {
                    pOutput[i] = old1;
                    pOutput[i + 1] = old2;
                } else {
                    pOutput[i] = new1 * cross_mix +
                                 old1 * (1.0 - cross_mix);
                    pOutput[i + 1] = new2  * cross_mix +
                                     old2 * (1.0 - cross_mix);
                    cross_mix += cross_inc;
                }
            }
            m_doRamping = false;
            m_doStart = false;
        }
    }

  protected:
    inline double processSample(double* coef, double* buf, double val);
    inline void pauseFilterInner() {
        // Set the current buffers to 0
        memset(m_buf1, 0, sizeof(m_buf1));
        memset(m_buf2, 0, sizeof(m_buf2));
        m_doRamping = true;
        m_doStart = true;
    }

    double m_coef[SIZE + 1];
    // Old coefficients needed for ramping
    double m_oldCoef[SIZE + 1];

    // Channel 1 state
    double m_buf1[SIZE];
    // Old channel 1 buffer needed for ramping
    double m_oldBuf1[SIZE];

    // Channel 2 state
    double m_buf2[SIZE];
    // Old channel 2 buffer needed for ramping
    double m_oldBuf2[SIZE];

    // Flag set to true if ramping needs to be done
    bool m_doRamping;
    // Flag set to true if old filter is invalid
    bool m_doStart;
    // Flag set to true if this is a chained filter
    bool m_startFromDry;
};
namespace {
    inline double processBQ(double *coef, double mcoef, double *buf, double val)
    {
        auto iir = val - buf[0] * coef[0] - buf[1] * coef[1];
        auto fir = iir + buf[1] * mcoef   + buf[0];
        buf[0] = buf[1];
        buf[1] = iir;
        return fir; 
    }
};
template<>
inline double EngineFilterIIR<2, IIR_LP>::processSample(double* coef,double* buf,double val) {
    return processBQ(&coef[1],2,&buf[0],val * coef[0]);
}
template<>
inline double EngineFilterIIR<2, IIR_BP>::processSample(double* coef,double* buf,double val) {
    return processBQ(&coef[1],0,&buf[0],val * coef[0]);
}
template<>
inline double EngineFilterIIR<2, IIR_HP>::processSample(double* coef,double* buf,double val) {
    return processBQ(&coef[1],-2,&buf[0],val * coef[0]);
}
template<>
inline double EngineFilterIIR<4, IIR_LP>::processSample(double* coef,double* buf,double val) {
    val = processBQ(&coef[1],2,&buf[0],val * coef[0]);
    val = processBQ(&coef[3],2,&buf[2],val);
    return val;
}
template<>
inline double EngineFilterIIR<8, IIR_BP>::processSample(double* coef,double* buf,double val) {
    val = processBQ(&coef[1],-2,&buf[0],val * coef[0]);
    val = processBQ(&coef[3],-2,&buf[2],val);
    val = processBQ(&coef[5],2,&buf[4],val);
    val = processBQ(&coef[7],2,&buf[6],val);
    return val;
}
template<>
inline double EngineFilterIIR<4, IIR_HP>::processSample(double* coef,double* buf,double val) {
    val = processBQ(&coef[1],-2,&buf[0],val * coef[0]);
    val = processBQ(&coef[3],-2,&buf[2],val);
    return val;
}
template<>
inline double EngineFilterIIR<8, IIR_LP>::processSample(double* coef,double* buf,double val) {
    val = processBQ(&coef[1],2,&buf[0],val * coef[0]);
    val = processBQ(&coef[3],2,&buf[2],val);
    val = processBQ(&coef[5],2,&buf[4],val);
    val = processBQ(&coef[7],2,&buf[6],val);
    return val;
}
template<>
inline double EngineFilterIIR<16, IIR_BP>::processSample(double* coef,double* buf,double val) {
    val = processBQ(&coef[1],-2,&buf[0],val * coef[0]);
    val = processBQ(&coef[3],-2,&buf[2],val);
    val = processBQ(&coef[5],-2,&buf[4],val);
    val = processBQ(&coef[7],-2,&buf[6],val);
    val = processBQ(&coef[9],2,&buf[8],val);
    val = processBQ(&coef[11],2,&buf[10],val);
    val = processBQ(&coef[13],2,&buf[12],val);
    val = processBQ(&coef[15],2,&buf[14],val);
    return val;
}

template<>
inline double EngineFilterIIR<8, IIR_HP>::processSample(double* coef,double* buf,double val) {
    val = processBQ(&coef[1],-2,&buf[0],val * coef[0]);
    val = processBQ(&coef[3],-2,&buf[2],val);
    val = processBQ(&coef[5],-2,&buf[4],val);
    val = processBQ(&coef[7],-2,&buf[6],val);
    return val;
}

// IIR_LP and IIR_HP use the same processSample routine
template<>
inline double EngineFilterIIR<5, IIR_BP>::processSample(double* coef,double* buf,double val) {
    auto iir = val * coef[0] - buf[0] * coef[1] - buf[1] * coef[3];
    auto fir = iir * coef[5] + buf[1] * coef[4] + buf[0] * coef[2];
    buf[0] = buf[1];
    buf[1] = iir;
    return fir;
}
