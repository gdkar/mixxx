#include "control/controlbehavior.h"
#include "control/control.h"
#include "util/math.h"

bool ControlNumericBehavior::setFilter(double* dValue) {
    Q_UNUSED(dValue);
    return true;
}

double ControlNumericBehavior::valueToParameter(double dValue) {
    return dValue;
}


double ControlNumericBehavior::parameterToValue(double dParam) {
    return dParam;
}


void ControlNumericBehavior::setValueFromParameter(double dParam,ControlDoublePrivate* pControl) {
    pControl->set(parameterToValue(dParam), NULL);
}

ControlPotmeterBehavior::ControlPotmeterBehavior(double dMinValue, double dMaxValue,
                                                 bool allowOutOfBounds)
        : m_dMinValue(dMinValue),
          m_dMaxValue(dMaxValue),
          m_dValueRange(m_dMaxValue - m_dMinValue),
          m_bAllowOutOfBounds(allowOutOfBounds) {
}

ControlPotmeterBehavior::~ControlPotmeterBehavior() {
}

bool ControlPotmeterBehavior::setFilter(double* dValue) {
    if (!m_bAllowOutOfBounds) {
        if (*dValue > m_dMaxValue) {
            *dValue = m_dMaxValue;
        } else if (*dValue < m_dMinValue) {
            *dValue = m_dMinValue;
        }
    }
    return true;
}

double ControlPotmeterBehavior::valueToParameter(double dValue) {
    if (m_dValueRange == 0.0) {
        return 0;
    }
    if (dValue > m_dMaxValue) {
        dValue = m_dMaxValue;
    } else if (dValue < m_dMinValue) {
        dValue = m_dMinValue;
    }
    return (dValue - m_dMinValue) / m_dValueRange;
}

double ControlPotmeterBehavior::parameterToValue(double dParam) {
    return m_dMinValue + (dParam * m_dValueRange);
}

#define maxPosition 1.0
#define minPosition 0.0
#define middlePosition ((maxPosition - minPosition) / 2.0)
#define positionrange (maxPosition - minPosition)

ControlLogPotmeterBehavior::ControlLogPotmeterBehavior(double dMinValue, double dMaxValue, double minDB)
        : ControlPotmeterBehavior(dMinValue, dMaxValue, false) {
    if (minDB >= 0) {
        qWarning() << "ControlLogPotmeterBehavior::ControlLogPotmeterBehavior() minDB must be negative";
        m_minDB = -1;
    } else {
        m_minDB = minDB;
    }
    m_minOffset = db2ratio(m_minDB);
}

ControlLogPotmeterBehavior::~ControlLogPotmeterBehavior() {
}

double ControlLogPotmeterBehavior::valueToParameter(double dValue) {
    if (m_dValueRange == 0.0) {
        return 0;
    }
    if (dValue > m_dMaxValue) {
        dValue = m_dMaxValue;
    } else if (dValue < m_dMinValue) {
        dValue = m_dMinValue;
    }
    double linPrameter = (dValue - m_dMinValue) / m_dValueRange;
    double dbParamter = ratio2db(linPrameter + m_minOffset * (1 - linPrameter));
    return 1 - (dbParamter / m_minDB);
}

double ControlLogPotmeterBehavior::parameterToValue(double dParam) {
    double dbParamter = (1 - dParam) * m_minDB;
    double linPrameter = (db2ratio(dbParamter) - m_minOffset) / (1 - m_minOffset);
    return m_dMinValue + (linPrameter * m_dValueRange);
}

ControlLinPotmeterBehavior::ControlLinPotmeterBehavior(double dMinValue, double dMaxValue,
                                                       bool allowOutOfBounds)
        : ControlPotmeterBehavior(dMinValue, dMaxValue, allowOutOfBounds) {
}

ControlLinPotmeterBehavior::~ControlLinPotmeterBehavior() {
}

ControlAudioTaperPotBehavior::ControlAudioTaperPotBehavior(
                             double minDB, double maxDB,
                             double neutralParameter)
        : ControlPotmeterBehavior(0.0, db2ratio(maxDB), false),
          m_neutralParameter(neutralParameter),
          m_minDB(minDB),
          m_maxDB(maxDB),
          m_offset(db2ratio(m_minDB)) {
}

ControlAudioTaperPotBehavior::~ControlAudioTaperPotBehavior() {
}

double ControlAudioTaperPotBehavior::valueToParameter(double dValue) {
    double dParam = 1.0;
    if (dValue <= 0.0) {
        return 0;
    } else if (dValue < 1.0) {
        // db + linear overlay to reach
        // m_minDB = 0
        // 0 dB = m_neutralParameter
        double overlay = m_offset * (1 - dValue);
        if (m_minDB) {
            dParam = (ratio2db(dValue + overlay) - m_minDB) / m_minDB * m_neutralParameter * -1;
        } else {
            dParam = dValue * m_neutralParameter;
        }
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
    if (dParam <= 0.0) {
        dValue = 0;
    } else if (dParam < m_neutralParameter) {
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
    pControl->set(parameterToValue(dParam), NULL);
}


double ControlTTRotaryBehavior::valueToParameter(double dValue) {
    return (dValue * 200.0 + 64) / 127.0;
}

double ControlTTRotaryBehavior::parameterToValue(double dParam) {
    dParam *= 128.0;
    // Non-linear scaling
    double temp = ((dParam - 64.0) * (dParam - 64.0)) / 64.0;
    if (dParam - 64 < 0) {
        temp = -temp;
    }
    return temp;
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
    // Calculate pressed State of the Button
    // Some controller like the RMX2 are sending always 
    // with a changed dParam 127 for pressed an 0 for released.
    // Other controller like the VMS4 are using 
    // And and a velocity value like a piano keyboard
    bool pressed = (dParam != 0);
    // This block makes push-buttons act as power window buttons.
    if (m_buttonMode == POWERWINDOW && m_iNumStates == 2) {
        if (pressed) {
            // Toggle on press
            double value = pControl->get();
            pControl->set(!value, NULL);
            m_pushTimer.setSingleShot(true);
            m_pushTimer.setTimerType(Qt::PreciseTimer);
            m_pushTimer.start(kPowerWindowTimeMillis);
        } else if (!m_pushTimer.isActive()) {
            // Disable after releasing a long press
            pControl->set(0., NULL);
        }
    } else if (m_buttonMode == TOGGLE || m_buttonMode == LONGPRESSLATCHING) {
        // This block makes push-buttons act as toggle buttons.
        if (m_iNumStates > 1) { // multistate button
            if (pressed) {
                // This is a possibly race condition if another writer wants
                // to change the value at the same time. We allow the race here,
                // because this is possibly what the user expects if he changes
                // the same control from different devices.
                double value = pControl->get();
                value = (int)(value + 1.) % m_iNumStates;
                pControl->set(value, NULL);
                if (m_buttonMode == LONGPRESSLATCHING) {
                    m_pushTimer.setSingleShot(true);
                    m_pushTimer.setTimerType(Qt::PreciseTimer);
                    m_pushTimer.start(kLongPressLatchingTimeMillis);
                }
            } else {
                double value = pControl->get();
                if (m_buttonMode == LONGPRESSLATCHING &&
                        m_pushTimer.isActive() && value >= 1.) {
                    // revert toggle if button is released too early
                    value = (int)(value - 1.) % m_iNumStates;
                    pControl->set(value, NULL);
                }
            }
        }
    } else { // Not a toggle button (trigger only when button pushed)
        if (pressed) {pControl->set(1., NULL);
        } else {pControl->set(0., NULL);}
    }
}
