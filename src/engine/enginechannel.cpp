/***************************************************************************
                          enginechannel.cpp  -  description
                             -------------------
    begin                : Sun Apr 28 2002
    copyright            : (C) 2002 by
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

#include "engine/enginechannel.h"

#include "control/controlobject.h"
#include "control/controlpushbutton.h"

EngineChannel::EngineChannel(const ChannelHandleAndGroup& handle_group,
                             EngineChannel::ChannelOrientation defaultOrientation, QObject*pParent)
        : EngineObject(pParent)
        , m_master(new CVAtom("master",this))
        , m_pfl(new CVAtom("pfl",this))
        , m_orientation(new CVAtom("orientation",this))
        , m_passthrough(new CVAtom("passthrough",this))
        , m_talkover(new CVAtom("talkover",this))
        , m_group(handle_group) {

    pParent->setProperty("master",QVariant::fromValue(m_master.data()));
    pParent->setProperty("pfl",QVariant::fromValue(m_pfl.data()));
    pParent->setProperty("orientation",QVariant::fromValue(m_orientation.data()));
    pParent->setProperty("passthrough",QVariant::fromValue(m_passthrough.data()));
    pParent->setProperty("talkover",QVariant::fromValue(m_talkover.data()));

    m_pPFL = new ControlPushButton(ConfigKey(getGroup(), "pfl"));
    m_pPFL->setButtonMode(ControlPushButton::TOGGLE);
    m_pMaster = new ControlPushButton(ConfigKey(getGroup(), "master"));
    m_pMaster->setButtonMode(ControlPushButton::TOGGLE);
    m_pMaster->set(1);
    m_pOrientation = new ControlPushButton(ConfigKey(getGroup(), "orientation"));
    m_pOrientation->setButtonMode(ControlPushButton::TOGGLE);
    m_pOrientation->setStates(3);
    m_pOrientation->set(defaultOrientation);
    m_pOrientationLeft = new ControlPushButton(ConfigKey(getGroup(), "orientation_left"));
    connect(m_pOrientationLeft, SIGNAL(valueChanged(double)),
            this, SLOT(slotOrientationLeft(double)), Qt::DirectConnection);
    m_pOrientationRight = new ControlPushButton(ConfigKey(getGroup(), "orientation_right"));
    connect(m_pOrientationRight, SIGNAL(valueChanged(double)),
            this, SLOT(slotOrientationRight(double)), Qt::DirectConnection);
    m_pOrientationCenter = new ControlPushButton(ConfigKey(getGroup(), "orientation_center"));
    connect(m_pOrientationCenter, SIGNAL(valueChanged(double)),
            this, SLOT(slotOrientationCenter(double)), Qt::DirectConnection);
    m_pTalkover = new ControlPushButton(ConfigKey(getGroup(), "talkover"));
    m_pTalkover->setButtonMode(ControlPushButton::POWERWINDOW);
}

EngineChannel::~EngineChannel() {
    delete m_pMaster;
    delete m_pPFL;
    delete m_pOrientation;
    delete m_pOrientationLeft;
    delete m_pOrientationRight;
    delete m_pOrientationCenter;
    delete m_pTalkover;
}

void EngineChannel::setPfl(bool enabled) {
    m_pfl->setValue(enabled ? 1.0 : 0.0);
}

bool EngineChannel::isPflEnabled() const {
    return !!m_pfl->getValue();
}

void EngineChannel::setMaster(bool enabled) {
    m_master->setValue(enabled ? 1.0 : 0.0);
}

bool EngineChannel::isMasterEnabled() const {
    return !!(m_master->getValue());
}

void EngineChannel::setTalkover(bool enabled) {
    
    m_talkover->setValue(enabled ? 1.0 : 0.0);
}

bool EngineChannel::isTalkoverEnabled() const {
    return m_talkover->getValue()>0;
}
void EngineChannel::setOrientation(ChannelOrientation o){
  m_orientation->setValue(o);
}
void EngineChannel::slotOrientationLeft(double v) {
    if (v > 0) {
        setOrientation(LEFT);
    }
}

void EngineChannel::slotOrientationRight(double v) {
    if (v > 0) {
        setOrientation(RIGHT);
    }
}

void EngineChannel::slotOrientationCenter(double v) {
    if (v > 0) {
        setOrientation(CENTER);
    }
}

EngineChannel::ChannelOrientation EngineChannel::getOrientation() const {
    double dOrientation = m_orientation->getValue();
    if (dOrientation == LEFT) {
        return LEFT;
    } else if (dOrientation == CENTER) {
        return CENTER;
    } else if (dOrientation == RIGHT) {
        return RIGHT;
    }
    return CENTER;
}
