/**
 * FilterPan Single :
 * + This delay will be applied on left channel following the
 * leftDelayFrames parameter and on the right one by minus leftDelayFrames.
 * + This delay is applied sample by sample and not on the full buffer.
 */

#ifndef ENGINEFILTERPANSINGLE_H
#define ENGINEFILTERPANSINGLE_H

#include <cstring>
#include <memory>
#include <utility>
#include <cstdio>
#include "engine/engineobject.h"
#include "util/assert.h"


class EngineFilterPanSingle {
    static const int numChannels = 2;
  public:
    EngineFilterPanSingle(size_t _SIZE)
            : SIZE(_SIZE),
              m_buf(std::make_unique<CSAMPLE[]>(_SIZE*numChannels)),
              m_delayFrame(0),
              m_doStart(false)
    {
        // Set the current buffers to 0
        std::fill_n(&m_buf[0], SIZE * numChannels , 0);
    }
    virtual ~EngineFilterPanSingle();
    void pauseFilter() {
        // Set the current buffers to 0
        if (!m_doStart) {
            std::fill_n(&m_buf[0], SIZE * numChannels , 0);
            m_doStart = true;
        }
    }
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput, double leftDelayFrames)
    {
        auto delayLeftSourceFrame = 0.;
        auto delayRightSourceFrame = 0.;

        if (leftDelayFrames > 0) {
            delayLeftSourceFrame = m_delayFrame + SIZE - leftDelayFrames;
            delayRightSourceFrame = m_delayFrame + SIZE;
        } else {
            delayLeftSourceFrame = m_delayFrame + SIZE;
            delayRightSourceFrame = m_delayFrame + SIZE + leftDelayFrames;
        }
        // put in samples into delay buffer
        m_buf[m_delayFrame * 2] = pIn[0];
        m_buf[m_delayFrame * 2 + 1] = pIn[1];
        // move the delay cursor forward
        m_delayFrame = (m_delayFrame + 1) % SIZE;

        // prepare coefficients for linear interpolation using a linear stretching
        auto timeBetweenFullSamplesLeft = std::fmod(delayLeftSourceFrame, 1);
        auto timeBetweenFullSamplesRight = std::fmod(delayRightSourceFrame, 1);

        // applying the delay on left channel with linear interpolation between each sample
        pOutput[0] = m_buf[(static_cast<int>(floor(delayLeftSourceFrame)) % SIZE) * 2] * (1 - timeBetweenFullSamplesLeft);
        pOutput[0] += m_buf[(static_cast<int>(ceil(delayLeftSourceFrame)) % SIZE) * 2] * timeBetweenFullSamplesLeft;
        // then on right channel
        pOutput[1] = m_buf[(static_cast<int>(floor(delayRightSourceFrame)) % SIZE) * 2 + 1] * (1 - timeBetweenFullSamplesRight);
        pOutput[1] += m_buf[(static_cast<int>(ceil(delayRightSourceFrame)) % SIZE) * 2 + 1] * timeBetweenFullSamplesRight;
        m_doStart = false;
    }
  protected:
    size_t SIZE;
    std::unique_ptr<CSAMPLE[]> m_buf;
    int m_delayFrame;
    bool m_doStart;
};

#endif // ENGINEFILTERPAN_H
