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

#include "controlpushbutton.h"
#include "controlpotmeter.h"
#include "controlobjectslave.h"
ControlPotmeter::ControlPotmeter(ConfigKey key, QObject*pParent)
        : ControlObject(key, true, false, false,pParent),
          m_controls(key) {
    setRange(0, 1.0, true);
    double default_value = 0.5;
    setDefaultValue(default_value);
    set(default_value);
    //qDebug() << "" << this << ", min " << m_dMinValue << ", max " << m_dMaxValue << ", range " << m_dValueRange << ", val " << m_dValue;
}
ControlPotmeter::ControlPotmeter(ConfigKey key, double dMinValue, double dMaxValue,
                                 bool allowOutOfBounds,
                                 bool bIgnoreNops,
                                 bool bTrack,
                                 bool bPersist,
                                 QObject*pParent)
        : ControlObject(key, bIgnoreNops, bTrack, bPersist,pParent),
          m_controls(key) {
    setRange(dMinValue, dMaxValue, allowOutOfBounds);
    double default_value = dMinValue + 0.5 * (dMaxValue - dMinValue);
    setDefaultValue(default_value);
    if (!bPersist) {set(default_value);}
    //qDebug() << "" << this << ", min " << m_dMinValue << ", max " << m_dMaxValue << ", range " << m_dValueRange << ", val " << m_dValue;
}
ControlPotmeter::~ControlPotmeter() {}
void ControlPotmeter::setStepCount(int count) {m_controls.setStepCount(count);}
void ControlPotmeter::setSmallStepCount(int count) {m_controls.setSmallStepCount(count);}
void ControlPotmeter::setRange(double dMinValue, double dMaxValue, bool allowOutOfBounds) {
    m_bAllowOutOfBounds = allowOutOfBounds;
    if (m_pControl) {m_pControl->setBehavior(new ControlPotmeterBehavior(dMinValue, dMaxValue, allowOutOfBounds));}
}
PotmeterControls::PotmeterControls(const ConfigKey& key,QObject*pParent)
        : QObject(pParent),
          m_pControl(new ControlObjectSlave(key,this)),
          m_stepCount(10),
          m_smallStepCount(100) {
    // These controls are deleted when the ControlPotmeter is since
    // PotmeterControls is a member variable of the associated ControlPotmeter
    // and the push-button controls are parented to the PotmeterControls.
    ControlPushButton* controlUp = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_up"),this);
    connect(controlUp, SIGNAL(valueChanged(double)),this, SLOT(incValue(double)));
    controlUp->setParent(this);
    ControlPushButton* controlDown = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_down"),this);
    connect(controlDown, SIGNAL(valueChanged(double)),this, SLOT(decValue(double)));
    controlDown->setParent(this);
    ControlPushButton* controlUpSmall = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_up_small"),this);
    connect(controlUpSmall, SIGNAL(valueChanged(double)),this, SLOT(incSmallValue(double)));
    controlUpSmall->setParent(this);
    ControlPushButton* controlDownSmall = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_down_small"),this);
    connect(controlDownSmall, SIGNAL(valueChanged(double)),this, SLOT(decSmallValue(double)));
    controlDownSmall->setParent(this);
    ControlPushButton* controlDefault = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_set_default"),this);
    connect(controlDefault, SIGNAL(valueChanged(double)),this, SLOT(setToDefault(double)));
    controlDefault->setParent(this);
    ControlPushButton* controlZero = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_set_zero"),this);
    connect(controlZero, SIGNAL(valueChanged(double)),this, SLOT(setToZero(double)));
    controlZero->setParent(this);
    ControlPushButton* controlOne = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_set_one"),this);
    connect(controlOne, SIGNAL(valueChanged(double)),this, SLOT(setToOne(double)));
    controlOne->setParent(this);
    ControlPushButton* controlMinusOne = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_set_minus_one"),this);
    connect(controlMinusOne, SIGNAL(valueChanged(double)),this, SLOT(setToMinusOne(double)));
    controlMinusOne->setParent(this);
    ControlPushButton* controlToggle = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_toggle"),this);
    connect(controlToggle, SIGNAL(valueChanged(double)),this, SLOT(toggleValue(double)));
    controlToggle->setParent(this);
    ControlPushButton* controlMinusToggle = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_minus_toggle"),this);
    connect(controlMinusToggle, SIGNAL(valueChanged(double)),this, SLOT(toggleMinusValue(double)));
    controlMinusToggle->setParent(this);
}
PotmeterControls::~PotmeterControls() {delete m_pControl;}
void PotmeterControls::incValue(double v) {
    if (v > 0) {
        double parameter = m_pControl->getParameter();
        parameter += 1.0 / m_stepCount;
        m_pControl->setParameter(parameter);
    }
}

void PotmeterControls::decValue(double v) {
    if (v > 0) {
        double parameter = m_pControl->getParameter();
        parameter -= 1.0 / m_stepCount;
        m_pControl->setParameter(parameter);
    }
}
void PotmeterControls::incSmallValue(double v) {
    if (v > 0) {
        double parameter = m_pControl->getParameter();
        parameter += 1.0 / m_smallStepCount;
        m_pControl->setParameter(parameter);
    }
}
void PotmeterControls::decSmallValue(double v) {
    if (v > 0) {
        double parameter = m_pControl->getParameter();
        parameter -= 1.0 / m_smallStepCount;
        m_pControl->setParameter(parameter);
    }
}
void PotmeterControls::setToZero(double v) {if (v > 0) {m_pControl->set(0.0);}}
void PotmeterControls::setToOne(double v) {if (v > 0) {m_pControl->set(1.0);}}
void PotmeterControls::setToMinusOne(double v) {if (v > 0) {m_pControl->set(-1.0);}}
void PotmeterControls::setToDefault(double v) {if (v > 0) {m_pControl->reset();}}
void PotmeterControls::toggleValue(double v) {
    if (v > 0) {
        double value = m_pControl->get();
        m_pControl->set(value > 0.0 ? 0.0 : 1.0);
    }
}
void PotmeterControls::toggleMinusValue(double v) {
    if (v > 0) {
        double value = m_pControl->get();
        m_pControl->set(value > 0.0 ? -1.0 : 1.0);
    }
}
