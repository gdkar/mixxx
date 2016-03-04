#include "engine/enginefilterdelay.h"
#include <utility>
#include <algorithm>
EngineFilterDelay::EngineFilterDelay(QObject *pParent, size_t _size)
        : EngineObjectConstIn(pParent),
            m_delaySamples(0),
            m_oldDelaySamples(0),
            m_delayPos(0),
            m_doRamping(false),
            m_doStart(false),
            m_size(roundup(_size)),
            m_buf(std::make_unique<CSAMPLE[]>(m_size)){
    // Set the current buffers to 0
    std::fill(&m_buf[0],&m_buf[m_size],0.f);
}
EngineFilterDelay::~EngineFilterDelay() = default;
void EngineFilterDelay::pauseFilter() {
    // Set the current buffers to 0
    if (!m_doStart) {
        std::fill(&m_buf[0],&m_buf[m_size],0.f);
        m_doStart = true;
    }
}
void EngineFilterDelay::setDelay(unsigned int delaySamples) {
    m_oldDelaySamples = m_delaySamples;
    m_delaySamples = delaySamples;
    m_doRamping = true;
}
void EngineFilterDelay::process(const CSAMPLE* pIn, CSAMPLE* pOutput,
                        const int iBufferSize) {
    if (!m_doRamping) {
        int delaySourcePos = (m_delayPos + m_size- m_delaySamples) &(m_size-1);

        DEBUG_ASSERT_AND_HANDLE(delaySourcePos >= 0) {
            SampleUtil::copy(pOutput, pIn, iBufferSize);
            return;
        }
        DEBUG_ASSERT_AND_HANDLE(delaySourcePos <= static_cast<int>(m_size)) {
            SampleUtil::copy(pOutput, pIn, iBufferSize);
            return;
        }

        for (int i = 0; i < iBufferSize; ++i) {
            // put sample into delay buffer:
            m_buf[m_delayPos] = pIn[i];
            m_delayPos = (m_delayPos + 1) &(m_size-1);

            // Take delayed sample from delay buffer and copy it to dest buffer:
            pOutput[i] = m_buf[delaySourcePos];
            delaySourcePos = (delaySourcePos + 1) &(m_size-1);
        }
    } else {
        int delaySourcePos = (m_delayPos + m_size - m_delaySamples + iBufferSize / 2) & (m_size-1);
        int oldDelaySourcePos = (m_delayPos + m_size- m_oldDelaySamples) &(m_size-1);

        DEBUG_ASSERT_AND_HANDLE(delaySourcePos >= 0) {
            SampleUtil::copy(pOutput, pIn, iBufferSize);
            return;
        }
        DEBUG_ASSERT_AND_HANDLE(delaySourcePos <= static_cast<int>(m_size)) {
            SampleUtil::copy(pOutput, pIn, iBufferSize);
            return;
        }
        DEBUG_ASSERT_AND_HANDLE(oldDelaySourcePos >= 0) {
            SampleUtil::copy(pOutput, pIn, iBufferSize);
            return;
        }
        DEBUG_ASSERT_AND_HANDLE(oldDelaySourcePos <= static_cast<int>(m_size)) {
            SampleUtil::copy(pOutput, pIn, iBufferSize);
            return;
        }

        CSAMPLE cross_mix = 0;
        CSAMPLE cross_inc = 2 / static_cast<CSAMPLE>(iBufferSize);

        for (auto i = 0; i < iBufferSize; ++i) {
            // put sample into delay buffer:
            m_buf[m_delayPos] = pIn[i];
            m_delayPos = (m_delayPos + 1) &(m_size-1);
            // Take delayed sample from delay buffer and copy it to dest buffer:
            if (i < iBufferSize / 2) {
                // only ramp the second half of the buffer, because we do
                // the same in the IIR filter to wait for settling
                pOutput[i] = m_buf[oldDelaySourcePos];
                oldDelaySourcePos = (oldDelaySourcePos + 1) &(m_size-1);
            } else {
                pOutput[i] = m_buf[delaySourcePos] * cross_mix;
                delaySourcePos = (delaySourcePos + 1) &(m_size-1);
                pOutput[i] += m_buf[oldDelaySourcePos] * (1.0 - cross_mix);
                oldDelaySourcePos = (oldDelaySourcePos + 1) &(m_size-1);
                cross_mix += cross_inc;
            }
        }
        m_doRamping = false;
    }
    m_doStart = false;
}


