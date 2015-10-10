#include "engine/enginefilterdelay.h"

EngineFilterDelay::EngineFilterDelay(int SIZE, QObject *pParent)
        : EngineObjectConstIn(QString{},pParent),
          m_size(SIZE),
          m_buf(std::make_unique<CSAMPLE[]>(m_size))
{
    std::fill(&m_buf[0],&m_buf[m_size],0);
}
EngineFilterDelay::~EngineFilterDelay() = default;
void EngineFilterDelay::pauseFilter()
{
    // Set the current buffers to 0
    if (!m_doStart)
    {
        std::fill(&m_buf[0],&m_buf[m_size],0);
        m_doStart = true;
    }
}
void EngineFilterDelay::setDelay(unsigned int delaySamples)
{
    m_oldDelaySamples = m_delaySamples;
    m_delaySamples = delaySamples;
    m_doRamping = true;
}
void EngineFilterDelay::process(const CSAMPLE* pIn, CSAMPLE* pOutput, int iBufferSize)
{
    if (!m_doRamping)
    {
        auto delaySourcePos = (m_delayPos + m_size - m_delaySamples) % m_size;
        DEBUG_ASSERT_AND_HANDLE(delaySourcePos >= 0)
        {
          std::copy_n(pIn,iBufferSize,pOutput);
          return;
        }
        DEBUG_ASSERT_AND_HANDLE(delaySourcePos <= static_cast<int>(m_size))
        {
            std::copy_n(pIn,iBufferSize,pOutput);
            return;
        }
        for (auto i = 0; i < iBufferSize; ++i)
        {
            // put sample into delay buffer:
            m_buf[m_delayPos] = pIn[i];
            m_delayPos = (m_delayPos + 1) % m_size;

            // Take delayed sample from delay buffer and copy it to dest buffer:
            pOutput[i] = m_buf[delaySourcePos];
            delaySourcePos = (delaySourcePos + 1) % m_size;
        }
    }
    else
    {
        auto delaySourcePos = (m_delayPos + m_size- m_delaySamples + iBufferSize / 2) % m_size;
        auto oldDelaySourcePos = (m_delayPos + m_size- m_oldDelaySamples) % m_size;
        DEBUG_ASSERT_AND_HANDLE(delaySourcePos >= 0)
        {
            std::copy_n(pIn,iBufferSize,pOutput);
            return;
        }
        DEBUG_ASSERT_AND_HANDLE(delaySourcePos <= static_cast<int>(m_size))
        {
            std::copy_n(pIn,iBufferSize,pOutput);
            return;
        }
        DEBUG_ASSERT_AND_HANDLE(oldDelaySourcePos >= 0)
        {
            std::copy_n(pIn,iBufferSize,pOutput);
            return;
        }
        DEBUG_ASSERT_AND_HANDLE(oldDelaySourcePos <= static_cast<int>(m_size))
        {
            std::copy_n(pIn,iBufferSize,pOutput);
            return;
        }
        auto cross_mix = static_cast<CSAMPLE_GAIN>(0.0);
        auto cross_inc = 2 / static_cast<CSAMPLE_GAIN>(iBufferSize);
        for (auto i = 0; i < iBufferSize; ++i)
        {
            // put sample into delay buffer:
            m_buf[m_delayPos] = pIn[i];
            m_delayPos = (m_delayPos + 1) % m_size;

            // Take delayed sample from delay buffer and copy it to dest buffer:
            if (i < iBufferSize / 2)
            {
                // only ramp the second half of the buffer, because we do
                // the same in the IIR filter to wait for settling
                pOutput[i] = m_buf[oldDelaySourcePos];
                oldDelaySourcePos = (oldDelaySourcePos + 1) % m_size;
            }
            else
            {
                pOutput[i] = m_buf[delaySourcePos] * cross_mix;
                delaySourcePos = (delaySourcePos + 1) % m_size;
                pOutput[i] += m_buf[oldDelaySourcePos] * (CSAMPLE_GAIN{1} - cross_mix);
                oldDelaySourcePos = (oldDelaySourcePos + 1) % m_size;
                cross_mix += cross_inc;
            }
        }
        m_doRamping = false;
    }
    m_doStart = false;
}

