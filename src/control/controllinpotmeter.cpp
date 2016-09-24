#include "control/controllinpotmeter.h"
ControlLinPotmeter::ControlLinPotmeter(ConfigKey key,
                                       double dMinValue, double dMaxValue,
                                       double dStep, double dSmallStep,
                                       bool allowOutOfBounds)
:ControlLinPotmeter(key,nullptr,dMinValue,dMaxValue,dStep,dSmallStep,allowOutOfBounds){}

ControlLinPotmeter::ControlLinPotmeter(ConfigKey key,QObject *p,
                                       double dMinValue, double dMaxValue,
                                       double dStep, double dSmallStep,
                                       bool allowOutOfBounds)
        : ControlPotmeter(key, p, dMinValue, dMaxValue, allowOutOfBounds) {
    if (m_pControl) {
        m_pControl->setBehavior(new ControlLinPotmeterBehavior(dMinValue, dMaxValue, allowOutOfBounds));
    }
    if (dStep) {
        setStepCount((dMaxValue - dMinValue) / dStep);
    }
    if (dSmallStep) {
        setSmallStepCount((dMaxValue - dMinValue) / dSmallStep);
    }
}
