#include "engine/enginefilteriir.h"

EngineFilterIIRBase::EngineFilterIIRBase(QObject *p)
    :EngineObjectConstIn(p)
{ }
EngineFilterIIRBase::~EngineFilterIIRBase() = default;
namespace {
    void processBQ(const CSAMPLE * pIn, CSAMPLE *pOut, CSAMPLE *coef, CSAMPLE mcoef, CSAMPLE *buf, const int count)
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
void EngineFilterIIR::processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize)
{
    process(pIn, pOutput, iBufferSize);
    SampleUtil::applyRampingGain(pOutput,1,0,iBufferSize);
    if(m_startFromDry)
        SampleUtil::addWithRampingGain(pOutput,pIn, 0, 1 , iBufferSize);
    pauseFilterInner();
}
void EngineFilterIIR::initBuffers()
{
    // Copy the current buffers into the old buffers
    m_oldBuf = m_buf;
    std::fill(m_buf.begin(),m_buf.end(), 0);
    // Set the current buffers to 0
    m_doRamping = true;
}
void EngineFilterIIR::setCoefs(double sampleRate, size_t n_coef1, QString spec, double freq0, double freq1, int adj)
{
    // Copy to dynamic-ish memory to prevent fidlib API breakage.
    // Copy the old coefficients into m_oldCoef
    if(n_coef1 == getSize() && freq0 == m_freq0 && freq1 == m_freq1 && spec == getSpec())
        return;

    if(!m_doRamping) {
        m_oldCoef   = m_coef;
        m_oldPASS   = PASS;
        m_oldBuf    = m_buf;
        m_doRamping = true;
    }
    SIZE = n_coef1;
    auto coef = std::vector<double>(n_coef1 + 1);
    coef[0] = m_fid.design_coef(&coef [1], n_coef1 ,spec.toLocal8Bit().constData(), sampleRate, freq0, freq1, adj);
    std::copy(coef.begin(),coef.end(),m_coef.begin());
    m_buf.resize(2 * SIZE);
    std::fill(m_buf.begin(),m_buf.end(), 0);
    m_freq0 = freq0;
    m_freq1 = freq1;
}
void EngineFilterIIR::assumeSettled()
{
    m_doRamping = false;
    m_doStart = false;
}
void EngineFilterIIR::processBuffer(const CSAMPLE *pIn, CSAMPLE *pOut, CSAMPLE *coef, CSAMPLE *buf, size_t size, const int count)
{
// IIR_LP and IIR_HP use the same processSample routine
   if(size == 5) {
        for(auto i = 0; i < count; i+=2) {
            auto iir0 = pIn[i + 0]* coef[0] - buf[0] * coef[1] - buf[2] * coef[3];
            auto iir1 = pIn[i + 1]* coef[0] - buf[1] * coef[1] - buf[3] * coef[3];
            pOut[i + 0] = iir0 * coef[5] + buf[2] * coef[4] + buf[0] * coef[2];
            pOut[i + 1] = iir1 * coef[5] + buf[3] * coef[4] + buf[1] * coef[2];
            buf[0] = buf[2];
            buf[1] = buf[3];
            buf[2] = iir0;
            buf[3] = iir1;
        }
   } else {
        auto mcoef = (PASS == IIR_BP && size == 2) ? 0.f : ((PASS == IIR_LP) ? 2.f : -2.f);
        processBQ(pIn,pOut,&coef[1], mcoef, &buf[0], count);
        for(auto i = size_t{2}; i < size; i += 2) {
            if(PASS == IIR_BP && i == (size/2))
                mcoef = 2.f;
            processBQ(pOut,pOut,&coef[1 + i], mcoef, &buf[i * 2], count);
        }
        SampleUtil::applyGain(pOut, coef[0], count);
    }
}
void EngineFilterIIR::process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize)
{
    processBuffer(pIn, pOutput, &m_coef[0], &m_buf[0], m_coef.size() - 1, iBufferSize);
    if(m_doRamping) {
        if(!m_doStart) {
            auto pTmp = static_cast<CSAMPLE*>(alloca(sizeof(CSAMPLE) * iBufferSize));
            std::swap(PASS,m_oldPASS);
            processBuffer(pIn, pTmp, &m_oldCoef[0], &m_oldBuf[0],m_oldCoef.size() - 1, iBufferSize);
            std::swap(PASS,m_oldPASS);
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
void EngineFilterIIR::pauseFilter()
{
    if(!m_doStart)
        pauseFilterInner();
}
void EngineFilterIIR::pauseFilterInner()
{
    // Set the current buffers to 0
    std::fill(m_buf.begin(),m_buf.end(),0.f);
    m_doRamping = true;
    m_doStart = true;
}
void EngineFilterIIR::setFrequencyCorners(double rate, double freq0, double freq1)
{
    setCoefs(rate, SIZE, m_spec, freq0, freq1);
}
EngineFilterIIR::EngineFilterIIR(QObject *pParent, size_t _SIZE, IIRPass _PASS, QString spec, QString tmpl)
    : EngineFilterIIRBase(pParent)
    , SIZE     (_SIZE)
    , PASS     (_PASS)
    , m_spec(spec)
    , m_tmpl(tmpl)
    , m_coef   (SIZE + 1)
    , m_buf    (SIZE * 2)
    , m_oldCoef(SIZE + 1)
    , m_oldBuf (SIZE * 2)
{
    pauseFilter();
}
EngineFilterIIR::EngineFilterIIR(size_t _SIZE, IIRPass _PASS, QString spec, QString tmpl)
    : EngineFilterIIR(nullptr, _SIZE, _PASS, spec, tmpl)
{ }
EngineFilterIIR::~EngineFilterIIR() = default;
void EngineFilterIIR::setStartFromDry(bool sfd)
{
    m_startFromDry = sfd;
}
bool EngineFilterIIR::getStartFromDry() const
{
    return m_startFromDry;
}
void EngineFilterIIR::setSpec(QString s)
{
    m_spec = s;
}
QString EngineFilterIIR::getSpec() const
{
    return m_spec;
}
void EngineFilterIIR::setTemplate(QString t)
{
    m_tmpl = t;
}
QString EngineFilterIIR::getTemplate() const
{
    return m_tmpl;
}
size_t EngineFilterIIR::getSize() const
{
    return SIZE;
}
IIRPass EngineFilterIIR::getPass() const
{
    return PASS;
}
void EngineFilterIIR::setPass(IIRPass _pass)
{
    if(_pass != getPass()) {
        if(!m_doRamping) {
            m_oldCoef   = m_coef;
            m_oldBuf    = m_buf;
            m_oldPASS   = PASS;
            m_doRamping = true;
        }
        PASS = _pass;
    }
}
