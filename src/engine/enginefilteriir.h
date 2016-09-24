_Pragma("once")
#include <cstdio>
#include <fidlib.hpp>

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
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize) = 0;
};

#define FIDSPEC_LENGTH 40

class EngineFilterIIR : public EngineFilterIIRBase {
    Q_OBJECT;
protected:
    Fid m_fid;
    size_t                    SIZE;
    IIRPass                   PASS;
    IIRPass                   m_oldPASS;
    double                    m_freq0;
    double                    m_freq1;
    QString                   m_spec;
    QString                   m_tmpl;
    std::vector<CSAMPLE>      m_coef;
    std::vector<CSAMPLE>      m_buf;
    std::vector<CSAMPLE>      m_oldCoef;
    std::vector<CSAMPLE>      m_oldBuf;

    bool m_doRamping = false;
    // Flag set to true if old filter is invalid
    bool m_doStart = false;
    // Flag set to true if this is a chained filter
    bool m_startFromDry= false;
  public:
    EngineFilterIIR(QObject *pParent, size_t _SIZE, IIRPass _PASS, QString spec, QString tmpl = QString{});
    EngineFilterIIR(size_t _SIZE, IIRPass _PASS, QString spec, QString tmpl = QString{});
    virtual ~EngineFilterIIR() ;
    void setStartFromDry(bool sfd);
    bool getStartFromDry() const;
    void setSpec(QString _sp);
    QString getSpec() const;
    void setTemplate(QString _tmpl);
    QString getTemplate() const;
    size_t getSize() const;
    IIRPass getPass() const;
    void setPass(IIRPass _pass);

    // this can be called continuously for Filters that have own ramping
    // or need no fade when disabling
    virtual void pauseFilter();
    // this is can be used instead off a final process() call before pause
    // It fades to dry or 0 according to the m_startFromDry parameter
    // it is an alternative for using pauseFillter() calls
    virtual void processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize);
    virtual void initBuffers();
    virtual void setCoefs(double sampleRate, size_t n_coef1, QString spec,double freq0, double freq1 = 0, int adj = 0);
    virtual void setFrequencyCorners(double rate, double freq0, double freq1 = 0);
    virtual void assumeSettled();
    virtual void processBuffer(const CSAMPLE *pIn, CSAMPLE *pOut, CSAMPLE *coef, CSAMPLE *buf, size_t size, const int count);
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize);
  protected:
    virtual void pauseFilterInner();
};
