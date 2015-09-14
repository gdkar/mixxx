/***************************************************************************
                          controlpushbutton.cpp  -  description
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
#include "control/control.h"
#include "control/controlbehavior.h"
/* -------- ------------------------------------------------------
   Purpose: Creates a new simulated latching push-button.
   Input:   key - Key for the configuration file
   -------- ------------------------------------------------------ */
ControlPushButton::ControlPushButton(ConfigKey key, bool bPersist,QObject *pParent)
        : ControlObject(key, false, false, bPersist),
          m_buttonMode(PUSH),
          m_iNoStates(2) {
    setParent(pParent);
    if (m_pControl) {
        m_pControl->setBehavior(new ControlPushButtonBehavior(static_cast<ControlPushButtonBehavior::ButtonMode>(m_buttonMode),m_iNoStates));
    }
}
ControlPushButton::~ControlPushButton() = default;
// Tell this PushButton how to act on rising and falling edges
void ControlPushButton::setButtonMode(enum ButtonMode mode) {
    //qDebug() << "Setting " << m_Key.group << m_Key.item << "as toggle";
    if(m_buttonMode != mode){
      m_buttonMode = mode;
      if (m_pControl) {
          m_pControl->setBehavior(new ControlPushButtonBehavior(static_cast<ControlPushButtonBehavior::ButtonMode>(m_buttonMode),m_iNoStates));
      }
      emit buttonModeChanged(mode);
    }
}
int ControlPushButton::numStates()const{return m_iNoStates;}
void ControlPushButton::setStates(int num_states) {
    if(m_iNoStates != num_states)
    {
      m_iNoStates = num_states;
      if (m_pControl) {
              m_pControl->setBehavior(new ControlPushButtonBehavior(static_cast<ControlPushButtonBehavior::ButtonMode>(m_buttonMode),m_iNoStates));
      }
      emit numStatesChanged(num_states);
    }
}
ControlPushButton::ButtonMode ControlPushButton::getButtonMode()const{return m_buttonMode;}
/*static */QString ControlPushButton::buttonModeToString(int mode) {
    switch(mode) {
        case ControlPushButton::PUSH:return "PUSH";
        case ControlPushButton::TOGGLE:return "TOGGLE";
        case ControlPushButton::POWERWINDOW:return "POWERWINDOW";
        case ControlPushButton::LONGPRESSLATCHING:return "LONGPRESSLATCHING";
        case ControlPushButton::TRIGGER:return "TRIGGER";
        default:return "UNKNOWN";
    }
}
