/***************************************************************************
                          controlpotmeter.cpp  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "control/controlpushbutton.h"
#include "control/controlpotmeter.h"
#include "control/controlproxy.h"
ControlPotmeter::ControlPotmeter(ConfigKey key,double dMinValue, double dMaxValue,
                                 bool bClampValue,
                                 bool bIgnoreNops,
                                 bool bTrack,
                                 bool bPersist)
:ControlPotmeter(key,nullptr,dMinValue,dMaxValue,bClampValue,bIgnoreNops,bTrack,bPersist){}
ControlPotmeter::ControlPotmeter(ConfigKey key,QObject *p,  double dMinValue, double dMaxValue,
                                 bool bClampValue,
                                 bool bIgnoreNops,
                                 bool bTrack,
                                 bool bPersist)
        : ControlObject(key, p, bIgnoreNops, bTrack, bPersist)
//          m_controls(key)
{
    setRange(dMinValue, dMaxValue, bClampValue);
    auto default_value = dMinValue + 0.5 * (dMaxValue - dMinValue);
    setDefaultValue(default_value);
    if (!bPersist) {
        set(default_value);
    }
    //qDebug() << "" << this << ", min " << m_dMinValue << ", max " << m_dMaxValue << ", range " << m_dValueRange << ", val " << m_dValue;
}
ControlPotmeter::~ControlPotmeter() = default;
void ControlPotmeter::setStepCount(int count)
{
    if(m_stepCount != count) {
        m_stepCount = count;//m_controls.setStepCount(count);
        emit stepCountChanged(count);
    }
}
void ControlPotmeter::setSmallStepCount(int count)
{
    if(m_smallStepCount != count) {
        m_smallStepCount = count;//m_controls.setSmallStepCount(count);
        emit smallStepCountChanged(count);
    }
}
void ControlPotmeter::setRange(double dMinValue, double dMaxValue,bool allowOutOfBounds)
{
    auto unconstrain_changed = m_bAllowOutOfBounds != allowOutOfBounds;
    m_bAllowOutOfBounds = allowOutOfBounds;
    auto min_changed = m_min != dMinValue;
    m_min = dMinValue;
    auto max_changed = m_max != dMaxValue;
    m_max = dMaxValue;
    if (m_pControl) {
        m_pControl->setBehavior(new ControlPotmeterBehavior(dMinValue, dMaxValue, allowOutOfBounds));
    }
    if(unconstrain_changed)
        unconstrainChanged(allowOutOfBounds);
    if(min_changed)
        minValueChanged(dMinValue);
    if(max_changed)
        maxValueChanged(dMaxValue);
}
double ControlPotmeter::minValue() const { return m_min;}
double ControlPotmeter::maxValue() const { return m_max;}
bool  ControlPotmeter::unconstrain() const { return m_bAllowOutOfBounds;}
int ControlPotmeter::stepCount() const { return m_stepCount;}
int ControlPotmeter::smallStepCount() const { return m_smallStepCount;}
void ControlPotmeter::setMinValue(double _min)
{
    if(_min != m_min) {
        setRange(_min, maxValue(), unconstrain());
    }
}
void ControlPotmeter::setMaxValue(double _max)
{
    if(_max != m_max) {
        setRange(minValue(), _max, unconstrain());
    }
}
void ControlPotmeter::setUnconstrain(bool x)
{
    if(m_bAllowOutOfBounds!= x) {
        setRange(minValue(), maxValue(), x);
    }
}

