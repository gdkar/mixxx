#include "control/controlbehavior.h"
#include "control/control.h"
#include "util/math.h"

bool ControlBehavior::setFilter(double* /*dValue*/)
{ 
  return true;
}
double ControlBehavior::valueToParameter(double dValue)
{ 
  return dValue;
}
double ControlBehavior::parameterToValue(double dParam)
{ 
  return dParam;
}
void ControlBehavior::setValueFromParameter(double dParam, ControlDoublePrivate* pControl)
{
    pControl->set(parameterToValue(dParam));
}
PotmeterBehavior::PotmeterBehavior(double dMinValue, double dMaxValue,bool allowOutOfBounds)
        : m_dMinValue(dMinValue),
          m_dMaxValue(dMaxValue),
          m_dValueRange(m_dMaxValue - m_dMinValue),
          m_bAllowOutOfBounds(allowOutOfBounds)
{
}
PotmeterBehavior::~PotmeterBehavior() = default;
bool PotmeterBehavior::setFilter(double* dValue)
{
    if (!m_bAllowOutOfBounds)
    {
        if (*dValue > m_dMaxValue)      *dValue = m_dMaxValue;
        else if (*dValue < m_dMinValue) *dValue = m_dMinValue;
    }
    return true;
}
double PotmeterBehavior::valueToParameter(double dValue)
{
    if (m_dValueRange == 0.0)        return 0;
    if (dValue > m_dMaxValue)        dValue = m_dMaxValue;
    else if (dValue < m_dMinValue)   dValue = m_dMinValue;
    return (dValue - m_dMinValue) / m_dValueRange;
}
double PotmeterBehavior::parameterToValue(double dParam)
{ 
  return m_dMinValue + (dParam * m_dValueRange);
}
#define maxPosition 1.0
#define minPosition 0.0
#define middlePosition ((maxPosition - minPosition) / 2.0)
#define positionrange (maxPosition - minPosition)
LogPotmeterBehavior::LogPotmeterBehavior(double dMinValue, double dMaxValue, double minDB)
        : PotmeterBehavior(dMinValue, dMaxValue, false)
{
    if (minDB >= 0)   m_minDB = -1;
    else              m_minDB = minDB;
    m_minOffset = db2ratio(m_minDB);
}
LogPotmeterBehavior::~LogPotmeterBehavior() = default;
double LogPotmeterBehavior::valueToParameter(double dValue)
{
    if (m_dValueRange == 0.0)      return 0;
    if (dValue > m_dMaxValue)      dValue = m_dMaxValue;
    else if (dValue < m_dMinValue) dValue = m_dMinValue;
    auto linPrameter = (dValue - m_dMinValue) / m_dValueRange;
    auto dbParamter = ratio2db(linPrameter + m_minOffset * (1 - linPrameter));
    return 1 - (dbParamter / m_minDB);
}
double LogPotmeterBehavior::parameterToValue(double dParam)
{
    auto dbParamter = (1 - dParam) * m_minDB;
    auto linPrameter = (db2ratio(dbParamter) - m_minOffset) / (1 - m_minOffset);
    return m_dMinValue + (linPrameter * m_dValueRange);
}
LinPotmeterBehavior::LinPotmeterBehavior(double dMinValue, double dMaxValue, bool allowOutOfBounds)
        : PotmeterBehavior(dMinValue, dMaxValue, allowOutOfBounds)
{
}
LinPotmeterBehavior::~LinPotmeterBehavior() = default;
AudioTaperPotBehavior::AudioTaperPotBehavior(
                             double minDB, double maxDB,
                             double neutralParameter)
        : PotmeterBehavior(0.0, db2ratio(maxDB), false),
          m_neutralParameter(neutralParameter),
          m_minDB(minDB),
          m_maxDB(maxDB),
          m_offset(db2ratio(m_minDB))
{
}
AudioTaperPotBehavior::~AudioTaperPotBehavior() = default;
double AudioTaperPotBehavior::valueToParameter(double dValue)
{
    auto dParam = 1.0;
    if (dValue <= 0.0) return 0;
    else if (dValue < 1.0)
    {
        // db + linear overlay to reach
        // m_minDB = 0
        // 0 dB = m_neutralParameter
        auto overlay = m_offset * (1 - dValue);
        if (m_minDB) dParam = (ratio2db(dValue + overlay) - m_minDB) / m_minDB * m_neutralParameter * -1;
        else dParam = dValue * m_neutralParameter;
    }
    else if (dValue == 1.0) dParam = m_neutralParameter;
    else if (dValue < m_dMaxValue)
    {
        // m_maxDB = 1
        // 0 dB = m_neutralParameter
        dParam = (ratio2db(dValue) / m_maxDB * (1 - m_neutralParameter)) + m_neutralParameter;
    }
    //qDebug() << "AudioTaperPotBehavior::valueToParameter" << "value =" << dValue << "dParam =" << dParam;
    return dParam;
}

double AudioTaperPotBehavior::parameterToValue(double dParam)
{
    auto dValue = 1.0;
    if (dParam <= 0.0) dValue = 0;
    else if (dParam < m_neutralParameter)
    {
        // db + linear overlay to reach
        // m_minDB = 0
        // 0 dB = m_neutralParameter;
        if (m_minDB)
        {
            auto db = (dParam * m_minDB / (m_neutralParameter * -1)) + m_minDB;
            dValue = (db2ratio(db) - m_offset) / (1 - m_offset) ;
        }
        else dValue = dParam / m_neutralParameter;
    }
    else if (dParam == m_neutralParameter) dValue = 1.0;
    else if (dParam <= 1.0)
    {
        // m_maxDB = 1
        // 0 dB = m_neutralParame;
        dValue = db2ratio((dParam - m_neutralParameter) * m_maxDB / (1 - m_neutralParameter));
    }
    //qDebug() << "AudioTaperPotBehavior::parameterToValue" << "dValue =" << dValue << "dParam =" << dParam;
    return dValue;
}
void AudioTaperPotBehavior::setValueFromParameter(double dParam,ControlDoublePrivate* pControl)
{ 
  pControl->set(parameterToValue(dParam));
}
double TTRotaryBehavior::valueToParameter(double dValue)
{ 
  return (dValue * 200.0 + 64) / 127.0;
}
double TTRotaryBehavior::parameterToValue(double dParam)
{
    dParam *= 128.0;
    // Non-linear scaling
    double temp = ((dParam - 64.0) * (dParam - 64.0)) / 64.0;
    if (dParam - 64 < 0) temp = -temp;
    return temp;
}
// static
const int PushButtonBehavior::kPowerWindowTimeMillis = 300;
const int PushButtonBehavior::kLongPressLatchingTimeMillis = 300;
PushButtonBehavior::PushButtonBehavior(ButtonMode buttonMode, int iNumStates)
        : m_buttonMode(buttonMode),
          m_iNumStates(iNumStates)
{
}
void PushButtonBehavior::setValueFromParameter(double dParam, ControlDoublePrivate* pControl)
{
    auto pressed = !!dParam;
    // This block makes push-buttons act as power window buttons.
    if (m_buttonMode == ButtonMode::PowerWindow && m_iNumStates == 2)
    {
        if (pressed)
        {
            // Toggle on press
            auto value = pControl->get();
            pControl->set(!value);
            m_pushTimer.setSingleShot(true);
            m_pushTimer.start(kPowerWindowTimeMillis);
        }
        else if (!m_pushTimer.isActive()) pControl->set(0.);
    }
    else if (m_buttonMode == ButtonMode::Toggle|| m_buttonMode == ButtonMode::Latching)
    {
        // This block makes push-buttons act as toggle buttons.
        if (m_iNumStates > 1)
        { // multistate button
            if (pressed)
            {
                // This is a possibly race condition if another writer wants
                // to change the value at the same time. We allow the race here,
                // because this is possibly what the user expects if he changes
                // the same control from different devices.
                auto value = pControl->get();
                value = (int)(value + 1.) % m_iNumStates;
                pControl->set(value);
                if (m_buttonMode == ButtonMode::Latching)
                {
                    m_pushTimer.setSingleShot(true);
                    m_pushTimer.start(kLongPressLatchingTimeMillis);
                }
            }
            else
            {
                auto value = pControl->get();
                if (m_buttonMode == ButtonMode::Latching&& m_pushTimer.isActive() && value >= 1.)
                {
                    // revert toggle if button is released too early
                    value = (int)(value - 1.) % m_iNumStates;
                    pControl->set(value);
                }
            }
        }
    } else pControl->set(pressed);
}
PushButtonBehavior::~PushButtonBehavior() = default;
