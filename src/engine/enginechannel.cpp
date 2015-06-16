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

#include "controlobject.h"
#include "controlpushbutton.h"

EngineChannel::EngineChannel(const ChannelHandleAndGroup& handle_group,
                             EngineChannel::ChannelOrientation defaultOrientation, QObject*pParent)
        : EngineObject(pParent),
          m_group(handle_group),
    m_pPFL (ConfigKey(getGroup(), "pfl")),
    m_pMaster (ConfigKey(getGroup(), "master")),
    m_pOrientation (ConfigKey(getGroup(), "orientation")),
    m_pOrientationLeft (ConfigKey(getGroup(), "orientation_left")),
    m_pOrientationRight (ConfigKey(getGroup(), "orientation_right")),
    m_pOrientationCenter(ConfigKey(getGroup(), "orientation_center")),
    m_pTalkover (ConfigKey(getGroup(), "talkover")){
    m_pTalkover.setButtonMode(ControlPushButton::POWERWINDOW);
    m_pMaster.setButtonMode(ControlPushButton::TOGGLE);
    m_pPFL.setButtonMode(ControlPushButton::TOGGLE);
    m_pOrientation.setButtonMode(ControlPushButton::TOGGLE);
    m_pOrientation.setStates(3);
    m_pOrientation.set(defaultOrientation);
    connect(&m_pOrientationLeft, SIGNAL(valueChanged(double)),
            this, SLOT(slotOrientationLeft(double)), Qt::DirectConnection);
    connect(&m_pOrientationRight, SIGNAL(valueChanged(double)),
            this, SLOT(slotOrientationRight(double)), Qt::DirectConnection);
    connect(&m_pOrientationCenter, SIGNAL(valueChanged(double)),
            this, SLOT(slotOrientationCenter(double)), Qt::DirectConnection);
}

EngineChannel::~EngineChannel() {
}
void EngineChannel::setPfl(bool enabled) {m_pPFL.set(enabled ? 1.0 : 0.0);}
bool EngineChannel::isPflEnabled() const {return m_pPFL.toBool();}
void EngineChannel::setMaster(bool enabled) {m_pMaster.set(enabled ? 1.0 : 0.0);}
bool EngineChannel::isMasterEnabled() const {return m_pMaster.toBool();}
void EngineChannel::setTalkover(bool enabled) {m_pTalkover.set(enabled ? 1.0 : 0.0);}
bool EngineChannel::isTalkoverEnabled() const {return m_pTalkover.toBool();}
void EngineChannel::slotOrientationLeft(double v) {if (v > 0) {m_pOrientation.set(LEFT);}}
void EngineChannel::slotOrientationRight(double v) {if (v > 0) {m_pOrientation.set(RIGHT);}}
void EngineChannel::slotOrientationCenter(double v) {if (v > 0) {m_pOrientation.set(CENTER);}}
EngineChannel::ChannelOrientation EngineChannel::getOrientation() const {
    double dOrientation = m_pOrientation.get();
    if (dOrientation == LEFT) {
        return LEFT;
    } else if (dOrientation == CENTER) {
        return CENTER;
    } else if (dOrientation == RIGHT) {
        return RIGHT;
    }
    return CENTER;
}
