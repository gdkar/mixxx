
#include "controlaudiotaperpot.h"

ControlAudioTaperPot::ControlAudioTaperPot(ConfigKey key,
                                           double minDB, double maxDB,
                                           double neutralParameter,
                                           QObject*pParent)
        : ControlPotmeter(key,0,1,false,true,false,false,pParent) {
    // Override ControlPotmeters default value of 0.5
    setDefaultValue(1.0);
    set(1.0);
    if (m_pControl) {
        m_pControl->setBehavior(new ControlAudioTaperPotBehavior(minDB, maxDB,neutralParameter));
    }
}
