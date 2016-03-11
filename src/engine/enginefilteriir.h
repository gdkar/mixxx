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
  virtual void setCoefs(double, size_t, QString spec, double, double, int) = 0;
  virtual void setCoefs2(double sampleRate
          , size_t n_coef1, const char* spec1, double freq01, double freq11, int adj1,
            size_t  n_coef2, const char* spec2, double freq02, double freq12, int adj2) = 0;
  virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize) = 0;
};

#define FIDSPEC_LENGTH 40

class EngineFilterIIR : public EngineFilterIIRBase {
    Q_OBJECT;
protected:
    size_t                    SIZE;
    IIRPass                   PASS;
    QString                   m_spec;
    std::vector<CSAMPLE>      m_coef;
    std::vector<CSAMPLE>      m_buf;
    std::vector<CSAMPLE>      m_oldCoef;
    std::vector<CSAMPLE>      m_oldBuf;
/*    double m_coef[SIZE + 1] = { 0. };
    // Old coefficients needed for ramping
    double m_oldCoef[SIZE + 1] = { 0. };
    // Channel 1 state
    double m_buf[SIZE * 2] = { 0. };
    // Old channel 1 buffer needed for ramping
    double m_oldBuf[SIZE * 2] = { 0. };*/

    bool m_doRamping = false;
    // Flag set to true if old filter is invalid
    bool m_doStart = false;
    // Flag set to true if this is a chained filter
    bool m_startFromDry= false;
  public:
    EngineFilterIIR(QObject *pParent, size_t _SIZE, IIRPass _PASS);
    EngineFilterIIR(size_t _SIZE, IIRPass _PASS);
    virtual ~EngineFilterIIR() ;
    // this can be called continuously for Filters that have own ramping
    // or need no fade when disabling
    virtual void pauseFilter();
    // this is can be used instead off a final process() call before pause
    // It fades to dry or 0 according to the m_startFromDry parameter
    // it is an alternative for using pauseFillter() calls
    void processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize);
    void initBuffers();
    void setCoefs(double sampleRate, size_t n_coef1, QString spec,double freq0, double freq1 = 0, int adj = 0);
    void setCoefs2(double sampleRate,
            size_t n_coef1, const char* spec1, double freq01, double freq11, int adj1,
            size_t n_coef2, const char* spec2, double freq02, double freq12, int adj2);
    virtual void assumeSettled();
    void processBuffer(const CSAMPLE *pIn, CSAMPLE *pOut, CSAMPLE *coef, CSAMPLE *buf, size_t size, const int count);
    void process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize);
  protected:
    void pauseFilterInner();
};
