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

ControlPotmeter::ControlPotmeter(ConfigKey key, double dMinValue, double dMaxValue,
                                 bool bClampValue,
                                 bool bIgnoreNops,
                                 bool bTrack,
                                 bool bPersist)
        : ControlObject(key, bIgnoreNops, bTrack, bPersist),
          m_controls(key)
{
    setRange(dMinValue, dMaxValue, bClampValue);
    auto default_value = dMinValue + 0.5 * (dMaxValue - dMinValue);
    setDefaultValue(default_value);
    if (!bPersist) {
        set(default_value);
    }
    //qDebug() << "" << this << ", min " << m_dMinValue << ", max " << m_dMaxValue << ", range " << m_dValueRange << ", val " << m_dValue;
}
ControlPotmeter::~ControlPotmeter() { }

void ControlPotmeter::setStepCount(int count)
{
    m_controls.setStepCount(count);
}

void ControlPotmeter::setSmallStepCount(int count)
{
    m_controls.setSmallStepCount(count);
}

void ControlPotmeter::setRange(double dMinValue, double dMaxValue,
                               bool allowOutOfBounds) {
    m_bAllowOutOfBounds = allowOutOfBounds;

    if (m_pControl) {
        m_pControl->setBehavior(
                new ControlPotmeterBehavior(dMinValue, dMaxValue, allowOutOfBounds));
    }
}

PotmeterControls::PotmeterControls(const ConfigKey& key)
        : m_pControl(new ControlProxy(key, this)),
          m_stepCount(10),
          m_smallStepCount(100) {
    // These controls are deleted when the ControlPotmeter is since
    // PotmeterControls is a member variable of the associated ControlPotmeter
    // and the push-button controls are parented to the PotmeterControls.

    auto controlUp = new ControlPushButton(
        ConfigKey(key.group, QString(key.item) + "_up"));
    controlUp->setParent(this);
    connect(controlUp, &ControlObject::valueChanged,this, &PotmeterControls::incValue);

    auto controlDown = new ControlPushButton(
        ConfigKey(key.group, QString(key.item) + "_down"));
    controlDown->setParent(this);
    connect(controlDown, &ControlObject::valueChanged,this, &PotmeterControls::decValue);

    auto controlUpSmall = new ControlPushButton(
        ConfigKey(key.group, QString(key.item) + "_up_small"));
    controlUpSmall->setParent(this);
    connect(controlUpSmall, &ControlObject::valueChanged,this, &PotmeterControls::incSmallValue);

    auto controlDownSmall = new ControlPushButton(
        ConfigKey(key.group, QString(key.item) + "_down_small"));
    controlDownSmall->setParent(this);
    connect(controlDownSmall, &ControlObject::valueChanged,this, &PotmeterControls::decSmallValue);

    auto controlDefault = new ControlPushButton(
        ConfigKey(key.group, QString(key.item) + "_set_default"));
    controlDefault->setParent(this);
    connect(controlDefault, &ControlObject::valueChanged,
            this, &PotmeterControls::setToDefault);

    auto controlToggle = new ControlPushButton(
        ConfigKey(key.group, QString(key.item) + "_toggle"));
    controlToggle->setParent(this);
    connect(controlToggle, &ControlObject::valueChanged,
            this, &PotmeterControls::toggleValue);

}

PotmeterControls::~PotmeterControls() {
}

void PotmeterControls::incValue(double v)
{
    if (v > 0) {
        m_pControl->fetch_add(1./m_stepCount);
    }
}
void PotmeterControls::decValue(double v)
{
    if (v > 0) {
        m_pControl->fetch_add(-1./m_stepCount);
    }
}
void PotmeterControls::incSmallValue(double v)
{
    if (v > 0) {
        m_pControl->fetch_add(1./m_smallStepCount);
    }
}
void PotmeterControls::decSmallValue(double v)
{
    if (v > 0) {
        m_pControl->fetch_add(-1./m_smallStepCount);
    }
}
void PotmeterControls::setToDefault(double v)
{
    if (v > 0) {
        m_pControl->reset();
    }
}
void PotmeterControls::toggleValue(double v)
{
    if (v > 0) {
        m_pControl->fetch_toggle();
    }
}
void PotmeterControls::setStepCount(int count)
{
    m_stepCount = count;
}
void PotmeterControls::setSmallStepCount(int count)
{
    m_smallStepCount = count;
}
