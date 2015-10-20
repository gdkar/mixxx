#include "controleffectknob.h"
#include "control/control.h"
#include "control/controlbehavior.h"
#include "util/math.h"
#include "effects/effectmanifestparameter.h"

ControlEffectKnob::ControlEffectKnob(ConfigKey key, double dMinValue, double dMaxValue)
        : ControlPotmeter(key, dMinValue, dMaxValue)
{
}

void ControlEffectKnob::setBehaviour(EffectManifestParameter::ControlHint type,double dMinValue, double dMaxValue)
{
    if (!m_pControl) return;
    if (type == EffectManifestParameter::CONTROL_KNOB_LINEAR) 
            m_pControl->setBehavior(new LinPotmeterBehavior(dMinValue, dMaxValue, false));
    else if (type == EffectManifestParameter::CONTROL_KNOB_LOGARITHMIC)
    {
        if (dMinValue == 0)
        {
            if (dMaxValue == 1.0) m_pControl->setBehavior(new AudioTaperPotBehavior(-20, 0, 1));
            else if (dMaxValue > 1.0) m_pControl->setBehavior(new AudioTaperPotBehavior(-12, ratio2db(dMaxValue), 0.5));
            else m_pControl->setBehavior(new LogPotmeterBehavior(dMinValue, dMaxValue, -40));
        }
        else m_pControl->setBehavior(new LogPotmeterBehavior(dMinValue, dMaxValue, -40));
    }
}
