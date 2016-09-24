
#include "control/controlaudiotaperpot.h"

ControlAudioTaperPot::ControlAudioTaperPot(ConfigKey key,QObject *pParent,
                                           double minDB, double maxDB,
                                           double neutralParameter)
        : ControlPotmeter(key,pParent) {

    // Override ControlPotmeters default value of 0.5
    setDefaultValue(1.0);
    set(1.0);
    if (m_pControl) {
        m_pControl->setBehavior(new ControlAudioTaperPotBehavior(minDB, maxDB,neutralParameter));
    }
}
