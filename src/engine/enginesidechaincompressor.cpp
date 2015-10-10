#include <QtDebug>

#include "engine/enginesidechaincompressor.h"

EngineSideChainCompressor::EngineSideChainCompressor(const char* /*group*/)
{
}

void EngineSideChainCompressor::calculateRates() {
    // Don't allow completely zero rates, or else if parameters change
    // we might get stuck on a compression value.
    if (!m_attackTime) m_attackPerFrame = m_strength;
    else               m_attackPerFrame = m_strength / m_attackTime;
    if (!m_decayTime)  m_decayPerFrame = m_strength;
    else               m_decayPerFrame = m_strength / m_decayTime;
    if (m_attackPerFrame <= 0) m_attackPerFrame = 0.005;
    if (m_decayPerFrame <= 0)  m_decayPerFrame = 0.005;
    qDebug() << "Compressor attack per frame: " << m_attackPerFrame
             << "decay per frame: " << m_decayPerFrame;
}
void EngineSideChainCompressor::clearKeys()
{
    m_bAboveThreshold = false;
}
void EngineSideChainCompressor::processKey(const CSAMPLE* pIn, const int iBufferSize) {
    for (auto i = 0; i + 1 < iBufferSize; i += 2) {
        auto val = (pIn[i] + pIn[i + 1]) * static_cast<CSAMPLE>(0.5);
        if (val > m_threshold) {
            m_bAboveThreshold = true;
            return;
        }
    }
}
double EngineSideChainCompressor::calculateCompressedGain(int frames) {
    if (m_bAboveThreshold) {
        if (m_compressRatio < m_strength) {
            m_compressRatio += m_attackPerFrame * frames;
            if (m_compressRatio > m_strength) m_compressRatio = m_strength;
        } else if (m_compressRatio > m_strength)
            m_compressRatio -= m_decayPerFrame * frames;
    } else {
        if (m_compressRatio > 0) {
            m_compressRatio -= m_decayPerFrame * frames;
            if (m_compressRatio < 0) m_compressRatio = 0;
        } else if (m_compressRatio < 0) {
            // Complain loudly.
            qWarning() << "Programming error, below-zero compression detected.";
            m_compressRatio += m_attackPerFrame * frames;
        }
    }
    return (1. - m_compressRatio);
}
void EngineSideChainCompressor::setParameters(CSAMPLE threshold, CSAMPLE strength, unsigned int attack_time, unsigned int decay_time)
{
    // TODO(owilliams): There is a race condition here because the parameters
    // are not updated atomically.  This function should instead take a
    // struct.
    m_threshold = threshold;
    m_strength = strength;
    m_attackTime = attack_time;
    m_decayTime = decay_time;
    calculateRates();
}
void EngineSideChainCompressor::setThreshold(CSAMPLE threshold)
{
    m_threshold = threshold;
    calculateRates();
}

void EngineSideChainCompressor::setStrength(CSAMPLE strength)
{
    m_strength = strength;
    calculateRates();
}
void EngineSideChainCompressor::setAttackTime(unsigned int attack_time)
{
    m_attackTime = attack_time;
    calculateRates();
}
void EngineSideChainCompressor::setDecayTime(unsigned int decay_time)
{
    m_decayTime = decay_time;
    calculateRates();
}
EngineSideChainCompressor::~EngineSideChainCompressor() = default;
