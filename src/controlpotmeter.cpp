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
#include "control/control.h"
ControlPotmeter::ControlPotmeter(ConfigKey key, double dMinValue, double dMaxValue,
                                 bool bAllowOutOfBounds,
                                 bool bIgnoreNops,
                                 bool bTrack,
                                 bool bPersist)
        : ControlObject(key, bIgnoreNops, bTrack, bPersist),
          m_controls(key) {
    setRange(dMinValue, dMaxValue, bAllowOutOfBounds);
    auto default_value = dMinValue + 0.5 * (dMaxValue - dMinValue);
    setDefaultValue(default_value);
    if (!bPersist) {set(default_value);}
    //qDebug() << "" << this << ", min " << m_dMinValue << ", max " << m_dMaxValue << ", range " << m_dValueRange << ", val " << m_dValue;
}
int ControlPotmeter::stepCount()const{return m_stepCount;}
int ControlPotmeter::smallStepCount()const{return m_smallStepCount;}
ControlPotmeter::~ControlPotmeter() = default;
void ControlPotmeter::setStepCount(int count) {
  if(count!=stepCount())
  {
    m_stepCount = count;
    m_controls.setStepCount(count);
    emit stepCountChanged(count);
  }
}
void ControlPotmeter::setSmallStepCount(int count) {
  if(count !=smallStepCount())
  {
    m_smallStepCount = count;
    m_controls.setSmallStepCount(count);
    emit smallStepCountChanged(count);
  }

}
void ControlPotmeter::setMaxValue(double v){setRange(minValue(),v,allowOutOfBounds());}
void ControlPotmeter::setMinValue(double v){setRange(v,maxValue(),allowOutOfBounds());}
void ControlPotmeter::setAllowOutOfBounds(bool b){setRange(minValue(),maxValue(),b);}
double ControlPotmeter::maxValue()const{return m_maxValue;}
double ControlPotmeter::minValue()const{return m_minValue;}
bool   ControlPotmeter::allowOutOfBounds()const{return m_bAllowOutOfBounds;}
void ControlPotmeter::setRange(double dMinValue, double dMaxValue,bool bAllowOutOfBounds) {
    if ( dMinValue != minValue() || dMaxValue != maxValue() || allowOutOfBounds() != bAllowOutOfBounds)
    {
      m_minValue = dMinValue;
      m_maxValue = dMaxValue;
      m_bAllowOutOfBounds = bAllowOutOfBounds;
      if (m_pControl) {m_pControl->setBehavior(new ControlPotmeterBehavior(dMinValue, dMaxValue, bAllowOutOfBounds));}
      emit rangeChanged();
    }
}
PotmeterControls::PotmeterControls(const ConfigKey& key,QObject *pParent)
        : QObject(pParent),
          m_pControl(new ControlObjectSlave(key,this)),
          m_stepCount(10),
          m_smallStepCount(100) {
    // These controls are deleted when the ControlPotmeter is since
    // PotmeterControls is a member variable of the associated ControlPotmeter
    // and the push-button controls are parented to the PotmeterControls.

    auto controlUp = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_up"));
    controlUp->setParent(this);
    connect(controlUp, SIGNAL(valueChanged(double)),this, SLOT(incValue(double)));

    auto controlDown = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_down"));
    controlDown->setParent(this);
    connect(controlDown, SIGNAL(valueChanged(double)),this, SLOT(decValue(double)));

    auto controlUpSmall = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_up_small"));
    controlUpSmall->setParent(this);
    connect(controlUpSmall, SIGNAL(valueChanged(double)),this, SLOT(incSmallValue(double)));

    auto controlDownSmall = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_down_small"));
    controlDownSmall->setParent(this);
    connect(controlDownSmall, SIGNAL(valueChanged(double)),this, SLOT(decSmallValue(double)));

    auto controlDefault = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_set_default"));
    controlDefault->setParent(this);
    connect(controlDefault, SIGNAL(valueChanged(double)),this, SLOT(setToDefault(double)));

    auto controlZero = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_set_zero"));
    controlZero->setParent(this);
    connect(controlZero, SIGNAL(valueChanged(double)),this, SLOT(setToZero(double)));

    auto controlOne = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_set_one"));
    controlOne->setParent(this);
    connect(controlOne, SIGNAL(valueChanged(double)),this, SLOT(setToOne(double)));

    auto controlMinusOne = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_set_minus_one"));
    controlMinusOne->setParent(this);
    connect(controlMinusOne, SIGNAL(valueChanged(double)),this, SLOT(setToMinusOne(double)));

    auto controlToggle = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_toggle"));
    controlToggle->setParent(this);
    connect(controlToggle, SIGNAL(valueChanged(double)),this, SLOT(toggleValue(double)));

    auto controlMinusToggle = new ControlPushButton(ConfigKey(key.group, QString(key.item) + "_minus_toggle"));
    controlMinusToggle->setParent(this);
    connect(controlMinusToggle, SIGNAL(valueChanged(double)),this, SLOT(toggleMinusValue(double)));
}
PotmeterControls::~PotmeterControls() = default;
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
void PotmeterControls::setStepCount(int c){m_stepCount=c;}
void PotmeterControls::setSmallStepCount(int c){m_smallStepCount=c;}
void PotmeterControls::setToZero(double v) {if (v > 0) {m_pControl->set(0.0);}}
void PotmeterControls::setToOne(double v) {if (v > 0) {m_pControl->set(1.0);}}
void PotmeterControls::setToMinusOne(double v) {if (v > 0) {m_pControl->set(-1.0);}}
void PotmeterControls::setToDefault(double v) {if (v > 0) {m_pControl->reset();}}
void PotmeterControls::toggleValue(double v) {if (v > 0) {m_pControl->set(m_pControl->get() > 0.0 ? 0.0 : 1.0);}}
void PotmeterControls::toggleMinusValue(double v) {if (v > 0) {m_pControl->set(m_pControl->get() > 0.0 ? -1.0 : 1.0);}}
