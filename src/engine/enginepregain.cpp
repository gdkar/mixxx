
#include <QtDebug>

#include "engine/enginepregain.h"
#include "controlaudiotaperpot.h"
#include "controlpotmeter.h"
#include "controlpushbutton.h"
#include "configobject.h"
#include "controlobject.h"
#include "util/math.h"
#include "sampleutil.h"

ControlPotmeter* EnginePregain::s_pReplayGainBoost = NULL;
ControlPotmeter* EnginePregain::s_pDefaultBoost = NULL;
ControlObject* EnginePregain::s_pEnableReplayGain = NULL;

/*----------------------------------------------------------------
   A pregaincontrol is ... a pregain.
   ----------------------------------------------------------------*/
EnginePregain::EnginePregain(const QString &group, QObject *pParent)
        : EngineObject(pParent),
          m_fSpeed(0),
          m_fOldSpeed(1.0),
          m_scratching(false),
          m_fPrevGain(1.0),
          m_pPotmeterPregain(ConfigKey(group,"pregain"),-12,12,0.5),
          m_pTotalGain(ConfigKey(group,"total_gain")),
          m_pCOReplayGain(ConfigKey(group,"replaygain")),
          m_pPassthroughEnabled(ConfigKey(group,"passthrough")),
          m_bSmoothFade(false) {
    //Replay Gain things
    if (s_pReplayGainBoost == NULL) {
        s_pReplayGainBoost = new ControlAudioTaperPot(ConfigKey("[ReplayGain]", "ReplayGainBoost"), -12, 12, 0.5);
        s_pDefaultBoost = new ControlAudioTaperPot(ConfigKey("[ReplayGain]", "DefaultBoost"), -12, 12, 0.5);
        s_pEnableReplayGain = new ControlObject(ConfigKey("[ReplayGain]", "ReplayGainEnabled"));
    }
}

EnginePregain::~EnginePregain() {
//    delete s_pEnableReplayGain;
//    s_pEnableReplayGain = NULL;
//    delete s_pReplayGainBoost;
//    s_pReplayGainBoost = NULL;
//    delete s_pDefaultBoost;
 //   s_pDefaultBoost = NULL;
}

void EnginePregain::setSpeed(CSAMPLE_GAIN  speed) {
    m_fOldSpeed = m_fSpeed;
    m_fSpeed = speed;
}

void EnginePregain::setScratching(bool scratching) {
    m_scratching = scratching;
}

void EnginePregain::process(CSAMPLE* pInOut, const int iBufferSize) {
    const CSAMPLE_GAIN fReplayGain = m_pCOReplayGain.get();
          CSAMPLE_GAIN fReplayGainCorrection;
    if (!s_pEnableReplayGain->toBool() || m_pPassthroughEnabled.toBool()) {
        // Override replaygain value if passing through
        // TODO(XXX): consider a good default.
        // Do we expect an replaygain leveled input or
        // Normalized to 1 input?
        fReplayGainCorrection = 1; // We expect a replaygain leveled input
    } else if (fReplayGain == 0) {
        // use predicted replaygain
        fReplayGainCorrection = (CSAMPLE_GAIN)s_pDefaultBoost->get();
        // We prepare for smoothfading to ReplayGain suggested gain
        // if ReplayGain value changes or ReplayGain is enabled
        m_bSmoothFade = true;
        m_timer.restart();
    } else {
        // Here is the point, when ReplayGain Analyser takes its action,
        // suggested gain changes from 0 to a nonzero value
        // We want to smoothly fade to this last.
        // Anyway we have some the problem that code cannot block the
        // full process for one second.
        // So we need to alter gain each time ::process is called.

        const CSAMPLE_GAIN fullReplayGainBoost = fReplayGain *
                static_cast<CSAMPLE_GAIN>(s_pReplayGainBoost->get());

        // This means that a ReplayGain value has been calculated after the
        // track has been loaded
        const CSAMPLE_GAIN  kFadeSeconds = 1.0;

        if (m_bSmoothFade) {
            CSAMPLE_GAIN seconds = static_cast<CSAMPLE_GAIN>(m_timer.elapsed()) * 1e-9;
            if (seconds < kFadeSeconds) {
                // Fade smoothly
                CSAMPLE fadeFrac = seconds / kFadeSeconds;
                fReplayGainCorrection = m_fPrevGain * (1.0 - fadeFrac) +
                        fadeFrac * fullReplayGainBoost;
            } else {
                m_bSmoothFade = false;
                fReplayGainCorrection = fullReplayGainBoost;
            }
        } else {
            // Passing a user defined boost
            fReplayGainCorrection = fullReplayGainBoost;
        }
    }

    // Clamp gain to within [0, 10.0] to prevent insane gains. This can happen
    // (some corrupt files get really high replay gain values).
    // 10 allows a maximum replay Gain Boost * calculated replay gain of ~2
    CSAMPLE_GAIN  totalGain = static_cast<CSAMPLE_GAIN>(m_pPotmeterPregain.get() *
            math_clamp(fReplayGainCorrection, 0.0f, 10.0f));

    m_pTotalGain.set(totalGain);

    // Vinylsoundemu:
    // As the speed approaches zero, hearing small bursts of sound at full volume
    // is distracting and doesn't mimic the way that vinyl sounds when played slowly.
    // Instead, reduce gain to provide a soft rolloff.
    const CSAMPLE_GAIN kThresholdSpeed = 0.070; // Scale volume if playback speed is below 7%.
    if (fabs(m_fSpeed) < kThresholdSpeed) {
        totalGain *= fabs(m_fSpeed) / kThresholdSpeed;
    }

    if ((m_fSpeed * m_fOldSpeed < 0) && m_scratching) {
        // direction changed, go though zero if scratching
        SampleUtil::applyRampingGain(&pInOut[0], m_fPrevGain, 0, iBufferSize / 2);
        SampleUtil::applyRampingGain(&pInOut[iBufferSize / 2], 0, totalGain, iBufferSize / 2);
    } else if (totalGain != m_fPrevGain) {
        // Prevent sound wave discontinuities by interpolating from old to new gain.
        SampleUtil::applyRampingGain(pInOut, m_fPrevGain, totalGain, iBufferSize);
        m_fPrevGain = totalGain;
    } else {
        // SampleUtil deals with aliased buffers and gains of 1 or 0.
        SampleUtil::applyGain(pInOut, totalGain, iBufferSize);
    }
}
