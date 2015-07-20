#include "control/controlbehavior.h"
#include "control/control.h"
#include "util/math.h"
bool ControlNumericBehavior::setFilter(double* dValue) {
    Q_UNUSED(dValue);
    return true;
}
double ControlNumericBehavior::valueToParameter(double dValue) {return dValue;}
double ControlNumericBehavior::parameterToValue(double dParam) {return dParam;}
void ControlNumericBehavior::setValueFromParameter(double dParam,ControlDoublePrivate* pControl) {
    const auto dNorm = valueToParameter(dParam);
    pControl->set(parameterToValue(dNorm), nullptr);
}
ControlPotmeterBehavior::ControlPotmeterBehavior(double dMinValue, double dMaxValue,bool allowOutOfBounds)
        : m_dMinValue(dMinValue),
          m_dMaxValue(dMaxValue),
          m_dValueRange(m_dMaxValue - m_dMinValue),
          m_bAllowOutOfBounds(allowOutOfBounds) {
}
ControlPotmeterBehavior::~ControlPotmeterBehavior() {}
bool ControlPotmeterBehavior::setFilter(double* dValue) {
    auto value = *dValue;
    if (!m_bAllowOutOfBounds) {
        value = std::min(m_dMaxValue,std::max(m_dMinValue,value));
        *dValue = value;
    }
    return true;
}
double ControlPotmeterBehavior::valueToParameter(double dValue) {
    if (m_dValueRange == 0.0) {return 0;}
    dValue = std::max(m_dMinValue,std::min(m_dMaxValue,dValue));
    return (dValue - m_dMinValue) / m_dValueRange;
}
double ControlPotmeterBehavior::parameterToValue(double dParam) {return m_dMinValue + (dParam * m_dValueRange);}

#define maxPosition 1.0
#define minPosition 0.0
#define middlePosition ((maxPosition - minPosition) / 2.0)
#define positionrange (maxPosition - minPosition)
ControlLogPotmeterBehavior::ControlLogPotmeterBehavior(double dMinValue, double dMaxValue, double minDB)
        : ControlPotmeterBehavior(dMinValue, dMaxValue, false) {
    if (minDB >= 0) {
        qWarning() << "ControlLogPotmeterBehavior::ControlLogPotmeterBehavior() minDB must be negative";
        m_minDB = -1;
    } else {m_minDB = minDB;}
    m_minOffset = db2ratio(m_minDB);
}
ControlLogPotmeterBehavior::~ControlLogPotmeterBehavior() {}
double ControlLogPotmeterBehavior::valueToParameter(double dValue) {
    if (m_dValueRange == 0.0) {return 0;}
    dValue = std::max(m_dMinValue,std::min(m_dMaxValue,dValue));
    auto linPrameter = (dValue - m_dMinValue) / m_dValueRange;
    auto  dbParamter = ratio2db(linPrameter + m_minOffset * (1 - linPrameter));
    return 1 - (dbParamter / m_minDB);
}
double ControlLogPotmeterBehavior::parameterToValue(double dParam) {
    auto   dbParamter = (1 - dParam) * m_minDB;
    auto  linPrameter = (db2ratio(dbParamter) - m_minOffset) / (1 - m_minOffset);
    return m_dMinValue + (linPrameter * m_dValueRange);
}
ControlLinPotmeterBehavior::ControlLinPotmeterBehavior(double dMinValue, double dMaxValue,bool allowOutOfBounds)
        : ControlPotmeterBehavior(dMinValue, dMaxValue, allowOutOfBounds) {
}
ControlLinPotmeterBehavior::~ControlLinPotmeterBehavior() {}
ControlAudioTaperPotBehavior::ControlAudioTaperPotBehavior(double minDB, double maxDB,double neutralParameter)
        : ControlPotmeterBehavior(0.0, db2ratio(maxDB), false),
          m_neutralParameter(neutralParameter),
          m_minDB(minDB),
          m_maxDB(maxDB),
          m_offset(db2ratio(m_minDB)) {
    m_midiCorrection = ceil(m_neutralParameter * 127) - (m_neutralParameter * 127);
}
ControlAudioTaperPotBehavior::~ControlAudioTaperPotBehavior() {}
double ControlAudioTaperPotBehavior::valueToParameter(double dValue) {
    double dParam = 1.0;
    if (dValue <= 0.0) {
        return 0;
    } else if (dValue < 1.0) {
        // db + linear overlay to reach
        // m_minDB = 0
        // 0 dB = m_neutralParameter
        double overlay = m_offset * (1 - dValue);
        if (m_minDB) {dParam = (ratio2db(dValue + overlay) - m_minDB) / m_minDB * m_neutralParameter * -1;}
        else {dParam = dValue * m_neutralParameter;}
    } else if (dValue == 1.0) {
        dParam = m_neutralParameter;
    } else if (dValue < m_dMaxValue) {
        // m_maxDB = 1
        // 0 dB = m_neutralParameter
        dParam = (ratio2db(dValue) / m_maxDB * (1 - m_neutralParameter)) + m_neutralParameter;
    }
    //qDebug() << "ControlAudioTaperPotBehavior::valueToParameter" << "value =" << dValue << "dParam =" << dParam;
    return dParam;
}
double ControlAudioTaperPotBehavior::parameterToValue(double dParam) {
    double dValue = 1;
    if (dParam <= 0.0) {dValue = 0;}
    else if (dParam < m_neutralParameter) {
        // db + linear overlay to reach
        // m_minDB = 0
        // 0 dB = m_neutralParameter;
        if (m_minDB) {
            double db = (dParam * m_minDB / (m_neutralParameter * -1)) + m_minDB;
            dValue = (db2ratio(db) - m_offset) / (1 - m_offset) ;
        } else {
            dValue = dParam / m_neutralParameter;
        }
    } else if (dParam == m_neutralParameter) {
        dValue = 1.0;
    } else if (dParam <= 1.0) {
        // m_maxDB = 1
        // 0 dB = m_neutralParame;
        dValue = db2ratio((dParam - m_neutralParameter) * m_maxDB / (1 - m_neutralParameter));
    }
    //qDebug() << "ControlAudioTaperPotBehavior::parameterToValue" << "dValue =" << dValue << "dParam =" << dParam;
    return dValue;
}

void ControlAudioTaperPotBehavior::setValueFromParameter(double dParam,ControlDoublePrivate* pControl) {
    auto dValue = parameterToValue(dParam);
    pControl->set(dValue, nullptr);
}
double ControlTTRotaryBehavior::valueToParameter(double dValue) { 
  auto temp = std::sqrt(std::abs(dValue)*0.5);
  return (dValue < 0)?(0.5 - temp):(0.5+temp);
}
double ControlTTRotaryBehavior::parameterToValue(double dParam) {
    // Non-linear scaling
    auto temp = ((dParam - 0.5) * (dParam - 0.5)) * 2;
    return (dParam < 0.5)? -temp:temp;;
}
// static
const int ControlPushButtonBehavior::kPowerWindowTimeMillis = 300;
const int ControlPushButtonBehavior::kLongPressLatchingTimeMillis = 300;
ControlPushButtonBehavior::ControlPushButtonBehavior(ButtonMode buttonMode,
                                                     int iNumStates)
        : m_buttonMode(buttonMode),
          m_iNumStates(iNumStates) {
}

void ControlPushButtonBehavior::setValueFromParameter(double dParam, ControlDoublePrivate* pControl) {
    auto pressed = (dParam>0);
    auto value = pControl->get();
    // This block makes push-buttons act as power window buttons.
    if (m_buttonMode == POWERWINDOW && m_iNumStates == 2) {
        if (pressed) {
            // Toggle on press
            pControl->set(!value, nullptr);
            m_pushTimer.setSingleShot(true);
            m_pushTimer.start(kPowerWindowTimeMillis);
            // Disable after releasing a long press
        } else if (!m_pushTimer.isActive()) {pControl->set(0., nullptr);}
    } else if (m_buttonMode == TOGGLE || m_buttonMode == LONGPRESSLATCHING) {
        // This block makes push-buttons act as toggle buttons.
        if (m_iNumStates > 1) { // multistate button
            if (pressed) {
                // This is a possibly race condition if another writer wants
                // to change the value at the same time. We allow the race here,
                // because this is possibly what the user expects if he changes
                // the same control from different devices.
                value = (int)(value + 1.) % m_iNumStates;
                pControl->set(value, nullptr);
                if (m_buttonMode == LONGPRESSLATCHING) {
                    m_pushTimer.setSingleShot(true);
                    m_pushTimer.start(kLongPressLatchingTimeMillis);
                }
            } else {
                if (m_buttonMode == LONGPRESSLATCHING && m_pushTimer.isActive() && value >= 1.) {
                    // revert toggle if button is released too early
                    value = (int)(value - 1.) % m_iNumStates;
                    pControl->set(value, nullptr);
                }
            }
        }
    } else { // Not a toggle button (trigger only when button pushed)
        pControl->set(pressed,nullptr);
    }
}
