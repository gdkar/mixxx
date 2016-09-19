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
#include "engine/enginevumeter.h"
#include "effects/effectsmanager.h"
#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "control/controlpushbutton.h"
#include "engine/effects/engineeffectsmanager.h"

EngineChannel::EngineChannel(QObject *p, const ChannelHandleAndGroup& handle_group,
                             EngineChannel::ChannelOrientation defaultOrientation,
                             EffectsManager *pEffectsManager)
        : EngineObject(p),m_group(handle_group) {
    m_pPFL = new ControlPushButton(ConfigKey(getGroup(), "pfl"));
    m_pPFL->setButtonMode(ControlPushButton::TOGGLE);
    m_pMaster = new ControlPushButton(ConfigKey(getGroup(), "master"));
    m_pMaster->setButtonMode(ControlPushButton::TOGGLE);
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
    m_pVUMeter = new EngineVuMeter(this,getGroup());
    if(pEffectsManager)
        pEffectsManager->registerChannel(handle_group);
    m_pEngineEffectsManager = pEffectsManager ? pEffectsManager->getEngineEffectsManager() : nullptr;
    m_pSampleRate = new ControlProxy(ConfigKey("[Master]","samplerate"),this);
    m_pInputConfigured = new ControlObject(ConfigKey(getGroup(),"input_configured"),this);
    m_wasActive = false;
    connect(m_pPFL, &ControlObject::valueChanged , this, &EngineChannel::pflEnabledChanged);
    connect(m_pMaster, &ControlObject::valueChanged, this, &EngineChannel::masterEnabledChanged);
    connect(m_pTalkover, &ControlObject::valueChanged, this, &EngineChannel::talkoverEnabledChanged);
    connect(m_pOrientation, &ControlObject::valueChanged, this, &EngineChannel::orientationChanged);
    connect(m_pInputConfigured,&ControlObject::valueChanged, this, &EngineChannel::activeChanged);
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
void EngineChannel::setOrientation(ChannelOrientation o)
{
    m_pOrientation->set(o);
}
void EngineChannel::setPfl(bool enabled) {
    m_pPFL->set(enabled ? 1.0 : 0.0);
}

bool EngineChannel::isPflEnabled() const {
    return m_pPFL->toBool();
}

void EngineChannel::setMaster(bool enabled) {
    m_pMaster->set(enabled ? 1.0 : 0.0);
}

bool EngineChannel::isMasterEnabled() const {
    return m_pMaster->toBool();
}

void EngineChannel::setTalkover(bool enabled) {
    m_pTalkover->set(enabled ? 1.0 : 0.0);
}

bool EngineChannel::isTalkoverEnabled() const {
    return m_pTalkover->toBool();
}

void EngineChannel::slotOrientationLeft(double v) {
    if (v > 0) {
        m_pOrientation->set(LEFT);
    }
}

void EngineChannel::slotOrientationRight(double v) {
    if (v > 0) {
        m_pOrientation->set(RIGHT);
    }
}

void EngineChannel::slotOrientationCenter(double v) {
    if (v > 0) {
        m_pOrientation->set(CENTER);
    }
}

EngineChannel::ChannelOrientation EngineChannel::getOrientation() const {
    double dOrientation = m_pOrientation->get();
    if (dOrientation == LEFT) {
        return LEFT;
    } else if (dOrientation == CENTER) {
        return CENTER;
    } else if (dOrientation == RIGHT) {
        return RIGHT;
    }
    return CENTER;
}
