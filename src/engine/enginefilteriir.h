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
    Q_OBJECT;
  public:
  Fid m_fid;
  EngineFilterIIRBase(QObject *p=nullptr);
  virtual ~EngineFilterIIRBase();
  virtual void assumeSettled() = 0;
  virtual void pauseFilter() = 0;
    virtual void processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize) = 0;
  virtual void initBuffers() = 0;
  virtual void setCoefs(const char*, double, double, double, int) = 0;
  virtual void setCoefs2(double sampleRate, int n_coef1,
            const char* spec1, double freq01, double freq11, int adj1,
            const char* spec2, double freq02, double freq12, int adj2) = 0;
  virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize) = 0;
};

#define FIDSPEC_LENGTH 40
namespace {
    inline void processBQ(const CSAMPLE * pIn, CSAMPLE *pOut, double *coef, double mcoef, double *buf, const int count)
    {
        for(auto i = 0; i < count; i += 2) {
            auto iir0   = pIn[i + 0] - buf[0] * coef[0] - buf[2] * coef[1];
            auto iir1   = pIn[i + 1] - buf[1] * coef[0] - buf[3] * coef[1];
            pOut[i + 0] = iir0       + buf[2] * mcoef   + buf[0];
            pOut[i + 1] = iir1       + buf[3] * mcoef   + buf[1];
            buf[0] = buf[2];
            buf[1] = buf[3];
            buf[2] = iir0;
            buf[3] = iir1;
        }
    }
};


template<unsigned int SIZE, IIRPass PASS>
class EngineFilterIIR : public EngineFilterIIRBase {
  public:
    EngineFilterIIR(QObject *pParent = nullptr)
        : EngineFilterIIRBase(pParent)
    {
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
        memcpy(m_oldBuf, m_buf, sizeof(m_buf));
        // Set the current buffers to 0
        memset(m_buf, 0, sizeof(m_buf));
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
    inline void processBuffer(const CSAMPLE *pIn, CSAMPLE *pOut, double *coef, double *buf, const int count)
    {
        auto mcoef = (PASS == IIR_LP) ? 2. : -2.;
        processBQ(pIn,pOut,&coef[1], mcoef, &buf[0], count);
        for(auto i = 2u; i < SIZE; i += 2) {
            if(PASS == IIR_BP && i == (SIZE/2))
                mcoef = 2.;
            processBQ(pOut,pOut,&coef[1 + i], mcoef, &buf[i * 2], count);
        }
        SampleUtil::applyGain(pOut, coef[0], count);
    }
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize) {
        processBuffer(pIn, pOutput, m_coef, m_buf, iBufferSize);
        if(m_doRamping) {
            if(!m_doStart) {
                auto pTmp = reinterpret_cast<CSAMPLE*>(alloca(sizeof(CSAMPLE) * iBufferSize));
                processBuffer(pIn, pTmp, m_oldCoef, m_oldBuf, iBufferSize);
                SampleUtil::copy(pOutput, pTmp, iBufferSize/2);
                SampleUtil::applyRampingGain(&pOutput[iBufferSize/2], 0.f, 1.f, iBufferSize/2);
                SampleUtil::addWithRampingGain(&pOutput[iBufferSize/2], &pTmp[iBufferSize/2], 1.f, 0.f, iBufferSize/2);
            } else {
                if(m_startFromDry) {
                    SampleUtil::copy(pOutput, pIn, iBufferSize/2);
                    SampleUtil::applyRampingGain(&pOutput[iBufferSize/2], 0.f, 1.f, iBufferSize/2);
                    SampleUtil::addWithRampingGain(&pOutput[iBufferSize/2], &pIn[iBufferSize/2], 1.f, 0.f, iBufferSize/2);
                } else {
                    SampleUtil::fill(pOutput, 0, iBufferSize/2);
                    SampleUtil::applyRampingGain(&pOutput[iBufferSize/2], 0.f, 1.f, iBufferSize/2);
                }
            }
            m_doRamping = false;
            m_doStart = false;
        }
    }
  protected:
    void pauseFilterInner()
    {
        // Set the current buffers to 0
        memset(m_buf, 0, sizeof(m_buf));
        m_doRamping = true;
        m_doStart = true;
    }
    double m_coef[SIZE + 1] = { 0. };
    // Old coefficients needed for ramping
    double m_oldCoef[SIZE + 1] = { 0. };
    // Channel 1 state
    double m_buf[SIZE * 2] = { 0. };
    // Old channel 1 buffer needed for ramping
    double m_oldBuf[SIZE * 2] = { 0. };

    bool m_doRamping = false;
    // Flag set to true if old filter is invalid
    bool m_doStart = false;
    // Flag set to true if this is a chained filter
    bool m_startFromDry= false;
};

// IIR_LP and IIR_HP use the same processSample routine
template<>
inline void EngineFilterIIR<5, IIR_BP>::processBuffer(const CSAMPLE *pIn, CSAMPLE *pOut, double* coef,double* buf,const int count) {
    for(int i = 0; i < count; i+=2) {
        auto iir0 = pIn[i + 0]* coef[0] - buf[0] * coef[1] - buf[2] * coef[3];
        auto iir1 = pIn[i + 1]* coef[0] - buf[1] * coef[1] - buf[3] * coef[3];
        pOut[i + 0] = iir0 * coef[5] + buf[2] * coef[4] + buf[0] * coef[2];
        pOut[i + 1] = iir1 * coef[5] + buf[3] * coef[4] + buf[1] * coef[2];
        buf[0] = buf[2];
        buf[1] = buf[3];
        buf[2] = iir0;
        buf[3] = iir1;
    }
}
template<>
inline void EngineFilterIIR<2,IIR_BP>::processBuffer(const CSAMPLE *pIn, CSAMPLE *pOut, double *coef, double *buf, const int count)
{
    processBQ(pIn,pOut,&coef[1], 0, &buf[0], count);
    SampleUtil::applyGain(pOut, coef[0], count);
}
