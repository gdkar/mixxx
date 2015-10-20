#include "controllinpotmeter.h"
#include "control/control.h"
#include "control/controlbehavior.h"

ControlLinPotmeter::ControlLinPotmeter(ConfigKey key,
                                       double dMinValue, double dMaxValue,
                                       double dStep, double dSmallStep,
                                       bool allowOutOfBounds)
        : ControlPotmeter(key, dMinValue, dMaxValue, allowOutOfBounds) {
    if (m_pControl) {m_pControl->setBehavior(new LinPotmeterBehavior(dMinValue, dMaxValue, allowOutOfBounds));}
    if (dStep) {setStepCount((dMaxValue - dMinValue) / dStep);}
    if (dSmallStep) {setSmallStepCount((dMaxValue - dMinValue) / dSmallStep);}
}



