#ifndef ENGINEFILTERDELAY_H
#define ENGINEFILTERDELAY_H

#include "engine/engineobject.h"
#include <memory>
#include <utility>
#include "util/assert.h"
#include "util/sample.h"

class EngineFilterDelay : public EngineObjectConstIn {
  public:
    EngineFilterDelay(QObject *p,size_t _SIZE)
            : EngineObjectConstIn(p),
              SIZE(_SIZE),
              m_buf(std::make_unique<CSAMPLE[]>(SIZE)),
              m_delaySamples(0),
              m_oldDelaySamples(0),
              m_delayPos(0),
              m_doRamping(false),
              m_doStart(false) {
        // Set the current buffers to 0
        std::fill_n(&m_buf[0],SIZE, 0);
    }
    virtual ~EngineFilterDelay() {};
    void pauseFilter() {
        // Set the current buffers to 0
        if (!m_doStart) {
            std::fill_n(&m_buf[0],SIZE, 0);
            m_doStart = true;
        }
    }
    void setDelay(unsigned int delaySamples) {
        m_oldDelaySamples = m_delaySamples;
        m_delaySamples = delaySamples;
        m_doRamping = true;
    }

    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,
                         const int iBufferSize) {
        if (!m_doRamping) {
            int delaySourcePos = (m_delayPos + SIZE - m_delaySamples) % SIZE;

            DEBUG_ASSERT_AND_HANDLE(delaySourcePos >= 0) {
                SampleUtil::copy(pOutput, pIn, iBufferSize);
                return;
            }
            DEBUG_ASSERT_AND_HANDLE(delaySourcePos <= static_cast<int>(SIZE)) {
                SampleUtil::copy(pOutput, pIn, iBufferSize);
                return;
            }

            for (int i = 0; i < iBufferSize; ++i) {
                // put sample into delay buffer:
                m_buf[m_delayPos] = pIn[i];
                m_delayPos = (m_delayPos + 1) % SIZE;

                // Take delayed sample from delay buffer and copy it to dest buffer:
                pOutput[i] = m_buf[delaySourcePos];
                delaySourcePos = (delaySourcePos + 1) % SIZE;
            }
        } else {
            int delaySourcePos = (m_delayPos + SIZE - m_delaySamples + iBufferSize / 2) % SIZE;
            int oldDelaySourcePos = (m_delayPos + SIZE - m_oldDelaySamples) % SIZE;

            DEBUG_ASSERT_AND_HANDLE(delaySourcePos >= 0) {
                SampleUtil::copy(pOutput, pIn, iBufferSize);
                return;
            }
            DEBUG_ASSERT_AND_HANDLE(delaySourcePos <= static_cast<int>(SIZE)) {
                SampleUtil::copy(pOutput, pIn, iBufferSize);
                return;
            }
            DEBUG_ASSERT_AND_HANDLE(oldDelaySourcePos >= 0) {
                SampleUtil::copy(pOutput, pIn, iBufferSize);
                return;
            }
            DEBUG_ASSERT_AND_HANDLE(oldDelaySourcePos <= static_cast<int>(SIZE)) {
                SampleUtil::copy(pOutput, pIn, iBufferSize);
                return;
            }

            CSAMPLE cross_mix = 0.0;
            CSAMPLE cross_inc = 2 / static_cast<CSAMPLE>(iBufferSize);

            for (int i = 0; i < iBufferSize; ++i) {
                // put sample into delay buffer:
                m_buf[m_delayPos] = pIn[i];
                m_delayPos = (m_delayPos + 1) % SIZE;

                // Take delayed sample from delay buffer and copy it to dest buffer:
                if (i < iBufferSize / 2) {
                    // only ramp the second half of the buffer, because we do
                    // the same in the IIR filter to wait for settling
                    pOutput[i] = m_buf[oldDelaySourcePos];
                    oldDelaySourcePos = (oldDelaySourcePos + 1) % SIZE;
                } else {
                    pOutput[i] = m_buf[delaySourcePos] * cross_mix;
                    delaySourcePos = (delaySourcePos + 1) % SIZE;
                    pOutput[i] += m_buf[oldDelaySourcePos] * (1.0 - cross_mix);
                    oldDelaySourcePos = (oldDelaySourcePos + 1) % SIZE;
                    cross_mix += cross_inc;
                }
            }
            m_doRamping = false;
        }
        m_doStart = false;
    }

  protected:
    size_t SIZE;
    std::unique_ptr<CSAMPLE[]> m_buf;
    int m_delaySamples;
    int m_oldDelaySamples;
    int m_delayPos;
    bool m_doRamping;
    bool m_doStart;
};

#endif // ENGINEFILTERDELAY_H
