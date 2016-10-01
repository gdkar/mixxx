#include "engine/enginefilteriir.h"

namespace {
    void processBQ(const CSAMPLE * pIn, CSAMPLE *pOut, CSAMPLE *coef, CSAMPLE mcoef, CSAMPLE *buf, int count)
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
void EngineFilterIIR::processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,int iBufferSize)
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
double EngineFilterIIR::getSampleRate() const
{
    return m_rate;
}
void EngineFilterIIR::setCoefs(double sampleRate, size_t n_coef1, QString spec, double freq0, double freq1, int adj)
{
    // Copy to dynamic-ish memory to prevent fidlib API breakage.
    // Copy the old coefficients into m_oldCoef
    auto rate_changed = m_rate != sampleRate;
    auto size_changed = n_coef1 != getSize();
    auto freq0_changed = freq0 != m_freq0;
    auto freq1_changed = freq1 != m_freq1;
    auto spec_changed  = spec != getSpec();
    auto adj_changed   = adj != m_adj;
    if(!rate_changed
    && !size_changed
    && !freq0_changed
    && !freq1_changed
    && !spec_changed
    && !adj_changed)
        return;

    if(!m_doRamping) {
        m_oldCoef   = m_coef;
        m_oldPass   = m_pass;
        m_oldBuf    = m_buf;
        m_doRamping = true;
    }
    m_size = n_coef1;
    auto coef = std::vector<double>(n_coef1 + 1);
    coef[0] = m_fid.design_coef(&coef [1], n_coef1 ,spec.toLocal8Bit().constData(), sampleRate, freq0, freq1, adj);
    std::copy(coef.begin(),coef.end(),m_coef.begin());
    m_buf.resize(2 * m_size);
    std::fill(m_buf.begin(),m_buf.end(), 0);
    m_spec = spec;
    m_adj   = adj;
    m_rate  = sampleRate;
    m_freq0 = freq0;
    m_freq1 = freq1;
    if(rate_changed)
        emit sampleRateChanged(m_rate);
    if(size_changed)
        emit sizeChanged(m_size);
    if(freq0_changed)
        emit freq0Changed(m_freq0);
    if(freq1_changed)
        emit freq1Changed(m_freq1);
    if(spec_changed)
        emit specChanged(m_spec);
}
void EngineFilterIIR::assumeSettled()
{
    m_doRamping = false;
    m_doStart = false;
}
void EngineFilterIIR::processBuffer(const CSAMPLE *pIn, CSAMPLE *pOut, CSAMPLE *coef, CSAMPLE *buf, size_t size, int count)
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
        auto mcoef = (m_pass == BandPass && size == 2) ? 0.f : ((m_pass == LowPass ) ? 2.f : -2.f);
        processBQ(pIn,pOut,&coef[1], mcoef, &buf[0], count);
        for(auto i = size_t{2}; i < size; i += 2) {
            if(m_pass == BandPass && i == (size/2))
                mcoef = 2.f;
            processBQ(pOut,pOut,&coef[1 + i], mcoef, &buf[i * 2], count);
        }
        SampleUtil::applyGain(pOut, coef[0], count);
    }
}
void EngineFilterIIR::process(const CSAMPLE* pIn, CSAMPLE* pOutput,int iBufferSize)
{
    processBuffer(pIn, pOutput, &m_coef[0], &m_buf[0], m_coef.size() - 1, iBufferSize);
    if(m_doRamping) {
        if(!m_doStart) {
            auto pTmp = static_cast<CSAMPLE*>(alloca(sizeof(CSAMPLE) * iBufferSize));
            std::swap(m_pass,m_oldPass);
            processBuffer(pIn, pTmp, &m_oldCoef[0], &m_oldBuf[0],m_oldCoef.size() - 1, iBufferSize);
            std::swap(m_pass,m_oldPass);
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
    setCoefs(rate, m_size, m_spec, freq0, freq1);
}
EngineFilterIIR::EngineFilterIIR(QObject *pParent, size_t _m_size, IIRPass _m_pass, QString spec, QString tmpl)
    : EngineObjectConstIn(pParent)
    , m_size     (_m_size)
    , m_pass     (_m_pass)
    , m_spec(spec)
    , m_tmpl(tmpl)
    , m_coef   (m_size + 1)
    , m_buf    (m_size * 2)
    , m_oldCoef(m_size + 1)
    , m_oldBuf (m_size * 2)
{
    pauseFilter();
}
EngineFilterIIR::EngineFilterIIR(size_t _m_size, IIRPass _m_pass, QString spec, QString tmpl)
    : EngineFilterIIR(nullptr, _m_size, _m_pass, spec, tmpl)
{ }
EngineFilterIIR::~EngineFilterIIR() = default;
void EngineFilterIIR::setStartFromDry(bool sfd)
{
    if(m_startFromDry != sfd)
        emit startFromDryChanged(m_startFromDry = sfd);
}
bool EngineFilterIIR::getStartFromDry() const
{
    return m_startFromDry;
}
double EngineFilterIIR::getFreq0() const
{
    return m_freq0;
}
double EngineFilterIIR::getFreq1() const
{
    return m_freq1;
}
void EngineFilterIIR::setFreq0(double f)
{
    if(m_freq0 != f) {
        setCoefs(m_rate,m_size, m_spec, f, m_freq1, m_adj);
    }
}
void EngineFilterIIR::setSampleRate(double rate)
{
    if(rate != m_rate) {
        setCoefs(rate,m_size, m_spec, m_freq0, m_freq1, m_adj);
    }
}
void EngineFilterIIR::setFreq1(double f)
{
    if(m_freq1 != f) {
        setCoefs(m_rate,m_size, m_spec, m_freq0, f, m_adj);
    }
}
void EngineFilterIIR::setSpec(QString s)
{
    if(m_spec != s) {
        setCoefs(m_rate,m_size, s, m_freq0, m_freq1, m_adj);
    }
}
QString EngineFilterIIR::getSpec() const
{
    return m_spec;
}
void EngineFilterIIR::setTemplate(QString t)
{
    if(m_tmpl != t)
        emit templateChanged(m_tmpl = t);
}
QString EngineFilterIIR::getTemplate() const
{
    return m_tmpl;
}
size_t EngineFilterIIR::getSize() const
{
    return m_size;
}
EngineFilterIIR::IIRPass EngineFilterIIR::getPass() const
{
    return m_pass;
}
void EngineFilterIIR::setPass(IIRPass _pass)
{
    if(_pass != getPass()) {
        if(!m_doRamping) {
            m_oldCoef   = m_coef;
            m_oldBuf    = m_buf;
            m_oldPass   = m_pass;
            m_doRamping = true;
        }
        m_pass = _pass;
        emit passChanged(_pass);
    }
}
