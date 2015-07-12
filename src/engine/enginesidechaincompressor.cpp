#include <QtDebug>
#include <cmath>
#include <ctgmath>

#include "engine/enginesidechaincompressor.h"

EngineSideChainCompressor::EngineSideChainCompressor(const QString&group,QObject *pParent)
        : EngineObject(pParent),
          m_compressRatioDb(0.0),
          m_bAboveThreshold(false),
          m_threshold(1.0),
          m_strength(0.0),
          m_attackTime(0),
          m_decayTime(0),
          m_attackPerFrame(0.0),
          m_decayPerFrame(0.0) {
    Q_UNUSED(pParent);
    Q_UNUSED(group);
}

void EngineSideChainCompressor::calculateRates() {
    // Don't allow completely zero rates, or else if parameters change
    // we might get stuck on a compression value.
    if (m_attackTime == 0) {
        // Attack really shouldn't be instant, but we allow it.
        m_attackPerFrame = m_strength;
    } else {
        m_attackPerFrame = m_strength / m_attackTime;
    }
    if (m_decayTime == 0) {
        m_decayPerFrame = m_strength;
    } else {
        m_decayPerFrame = m_strength / m_decayTime;
    }
    if (m_attackPerFrame <= 0) {
        m_attackPerFrame = 0.005;
    }
    if (m_decayPerFrame <= 0) {
        m_decayPerFrame = 0.005;
    }
    qDebug() << "Compressor attack per frame: " << m_attackPerFrame << "decay per frame: " << m_decayPerFrame;
}
void EngineSideChainCompressor::clearKeys() {m_bAboveThreshold = false; m_compressRatioDb = 0.0;m_currentMag = 0.0;}
void EngineSideChainCompressor::process(CSAMPLE* pIn, const int iBufferSize) {
    auto left_acc = CSAMPLE{0};
    auto right_acc = CSAMPLE{0};
    for (auto i = 0; i + 1 < iBufferSize; i += 2)
    {
      left_acc  += pIn[2*i+0];
      right_acc += pIn[2*i+1];
    }
    left_acc  /= (0.5*iBufferSize);
    right_acc /= (0.5*iBufferSize);
    auto left_mag = CSAMPLE{0};
    auto right_mag = CSAMPLE{0};
    for(auto i = 0; i + 1 < iBufferSize; i+=2)
    {
      left_mag  = math_max ( left_mag,  std::abs(pIn[2*i+0] -  left_acc));
      right_mag = math_max ( right_mag, std::abs(pIn[2*i+1] - right_acc));
    }
    m_currentMag = ( math_max(left_mag,right_mag)/m_threshold);
    m_bAboveThreshold = m_currentMag > 1;
}

double EngineSideChainCompressor::calculateCompressedGain(int frames) {
    double currentDb         = ratio2db(std::max<double>(m_currentMag,1.0));
    double m_compressTargetDb = (m_strength - 1.0) * currentDb;
    if( m_compressTargetDb < m_compressRatioDb )
    {
      m_compressRatioDb += ( m_compressTargetDb - m_compressRatioDb ) * frames / m_attackTime;
    }else{
      m_compressRatioDb += ( m_compressTargetDb - m_compressRatioDb ) * frames / m_decayTime;
    }
    return db2ratio ( m_compressRatioDb );
/*    if (m_bAboveThreshold) {
        if (m_compressRatio < m_strength) {
            m_compressRatio += m_attackPerFrame * frames;
            if (m_compressRatio > m_strength) {
                // If we overshot, clamp.
                m_compressRatio = m_strength;
            }
        } else if (m_compressRatio > m_strength) {
            // If the strength param was changed, we might be compressing too much.
            m_compressRatio -= m_decayPerFrame * frames;
        }
    } else {
        if (m_compressRatio > 0) {
            m_compressRatio -= m_decayPerFrame * frames;
            if (m_compressRatio < 0) {
                // If we overshot, clamp.
                m_compressRatio = 0;
            }
        } else if (m_compressRatio < 0) {
            // Complain loudly.
            qWarning() << "Programming error, below-zero compression detected.";
            m_compressRatio += m_attackPerFrame * frames;
        }
    }
    return (1. - m_compressRatio);*/
}
