#ifndef ENGINEFILTERPAN_H
#define ENGINEFILTERPAN_H

#include <cstring>
#include <utility>
#include <algorithm>
#include <memory>
#include <cstdint>
#include "engine/engineobject.h"
#include "util/assert.h"
#include "util/sample.h"


class EngineFilterPan : public EngineObjectConstIn {
  static const int numChannels = 2;
  public:
    EngineFilterPan(QObject *p, size_t _SIZE)
            : EngineObjectConstIn(p),
              SIZE(_SIZE),
              m_buf(std::make_unique<CSAMPLE[]>(SIZE * numChannels)),
              m_leftDelayFrames(0),
              m_oldLeftDelayFrames(0),
              m_delayFrame(0),
              m_doRamping(false),
              m_doStart(false) {
        // Set the current buffers to 0
        std::fill_n(&m_buf[0], SIZE * numChannels, 0);
    }
   ~EngineFilterPan();
    void pauseFilter()
    {
        // Set the current buffers to 0
        if (!m_doStart) {
            std::fill_n(&m_buf[0], SIZE * numChannels, 0);
            m_doStart = true;
        }
    }
    void setLeftDelay(int leftDelaySamples)
    {
        if (m_leftDelayFrames != leftDelaySamples) {
            m_oldLeftDelayFrames = m_leftDelayFrames;
            m_leftDelayFrames = leftDelaySamples;
            m_doRamping = true;
        }
    }
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize)
    {
        auto delayLeftSourceFrame = 0;
        auto delayRightSourceFrame = 0;
        if (m_leftDelayFrames > 0) {
            delayLeftSourceFrame = m_delayFrame + SIZE - m_leftDelayFrames;
            delayRightSourceFrame = m_delayFrame + SIZE;
        } else {
            delayLeftSourceFrame = m_delayFrame + SIZE;
            delayRightSourceFrame = m_delayFrame + SIZE + m_leftDelayFrames;
        }
        DEBUG_ASSERT_AND_HANDLE(delayLeftSourceFrame >= 0 && delayRightSourceFrame >= 0) {
            SampleUtil::copy(pOutput, pIn, iBufferSize);
            return;
        }
        if (!m_doRamping) {
            for (auto i = 0; i < iBufferSize / 2; ++i) {
                // put sample into delay buffer:
                m_buf[m_delayFrame * 2 + 0] = pIn[i * 2 + 0];
                m_buf[m_delayFrame * 2 + 1] = pIn[i * 2 + 1];
                m_delayFrame = (m_delayFrame + 1) % SIZE;

                // Take delayed sample from delay buffer and copy it to dest buffer:
                pOutput[i * 2 + 0] = m_buf[(delayLeftSourceFrame % SIZE) * 2 + 0];
                pOutput[i * 2 + 1] = m_buf[(delayRightSourceFrame % SIZE) * 2 + 1];
                delayLeftSourceFrame++;
                delayRightSourceFrame++;
            }
        } else {
            auto delayOldLeftSourceFrame = 0;
            auto delayOldRightSourceFrame = 0;
            if (m_oldLeftDelayFrames > 0) {
                delayOldLeftSourceFrame = m_delayFrame + SIZE - m_oldLeftDelayFrames;
                delayOldRightSourceFrame = m_delayFrame + SIZE;
            } else {
                delayOldLeftSourceFrame = m_delayFrame + SIZE;
                delayOldRightSourceFrame = m_delayFrame + SIZE + m_oldLeftDelayFrames;
            }

            DEBUG_ASSERT_AND_HANDLE(delayOldLeftSourceFrame >= 0 && delayOldRightSourceFrame >= 0) {
                SampleUtil::copy(pOutput, pIn, iBufferSize);
                return;
            }
            auto cross_mix = 0.0;
            auto cross_inc = 2 / static_cast<double>(iBufferSize);
            for (auto i = 0; i < iBufferSize / 2; ++i) {
                // put sample into delay buffer:
                m_buf[m_delayFrame * 2] = pIn[i * 2];
                m_buf[m_delayFrame * 2 + 1] = pIn[i * 2 + 1];
                m_delayFrame = (m_delayFrame + 1) % SIZE;

                // Take delayed sample from delay buffer and copy it to dest buffer:
                cross_mix += cross_inc;

                auto rampedLeftSourceFrame = delayLeftSourceFrame * cross_mix + delayOldLeftSourceFrame * (1 - cross_mix);
                auto rampedRightSourceFrame = delayRightSourceFrame * cross_mix + delayOldRightSourceFrame * (1 - cross_mix);
                auto modLeft = fmod(rampedLeftSourceFrame, 1);
                auto modRight = fmod(rampedRightSourceFrame, 1);

                pOutput[i * 2] = m_buf[(static_cast<int>(floor(rampedLeftSourceFrame)) % SIZE) * 2] * (1 - modLeft);
                pOutput[i * 2 + 1] = m_buf[(static_cast<int>(floor(rampedRightSourceFrame)) % SIZE) * 2 + 1] * (1 - modRight);
                pOutput[i * 2] += m_buf[(static_cast<int>(ceil(rampedLeftSourceFrame)) % SIZE) * 2] * modLeft;
                pOutput[i * 2 + 1] += m_buf[(static_cast<int>(ceil(rampedRightSourceFrame)) % SIZE) * 2 + 1] * modRight;
                delayLeftSourceFrame++;
                delayRightSourceFrame++;
                delayOldLeftSourceFrame++;
                delayOldRightSourceFrame++;
            }
            m_doRamping = false;
        }
        m_doStart = false;
    }

  protected:
    size_t SIZE;
    std::unique_ptr<CSAMPLE[]> m_buf;
    int m_leftDelayFrames;
    int m_oldLeftDelayFrames;
    int m_delayFrame;
    bool m_doRamping;
    bool m_doStart;
};

#endif // ENGINEFILTERPAN_H
