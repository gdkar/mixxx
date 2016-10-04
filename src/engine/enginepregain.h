#ifndef ENGINEPREGAIN_H
#define ENGINEPREGAIN_H

#include "engine/engineobject.h"
#include "control/controlobject.h"
#include "util/performancetimer.h"

class ControlObject;

// The pregain control alters the volume of the track based on several inputs,
// including user pregain adjustment, ReplayGain value, and vinyl-like
// adjustments in volume relative to playback speed.
class EnginePregain : public EngineObject {
  public:
    EnginePregain(QObject *p, QString group);
    virtual ~EnginePregain();

    void setSpeed(double speed);

    // If the user is scratching and the record reverses direction, the volume
    // will be ramped to zero and back up again to mimic a vinyl scratch.
    // If the user is not scratching and the direction is reversed
    // (e.g. reverse button is pressed), the audio will be immediately
    // reversed without a ramp to zero.
    void setScratching(bool scratching);

    void process(CSAMPLE* pInOut, int iBufferSize);

  private:
    double m_dSpeed;
    double m_dOldSpeed;
    bool m_scratching;
    float m_fPrevGain;
    ControlObject* m_pPotmeterPregain;
    ControlObject* m_pTotalGain;
    ControlObject* m_pCOReplayGain;
    ControlObject* m_pPassthroughEnabled;
    static ControlObject* s_pReplayGainBoost;
    static ControlObject* s_pDefaultBoost;
    static ControlObject* s_pEnableReplayGain;
    bool m_bSmoothFade;
    PerformanceTimer m_timer;
};

#endif
