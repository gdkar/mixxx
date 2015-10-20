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
ControlPushButton::ControlPushButton(ConfigKey key, QObject *pParent,bool bPersist)
        : ControlObject(key, pParent, false, bPersist)
{
    setParent(pParent);
    qRegisterMetaType<ButtonMode>();
    if (m_pControl) 
        m_pControl->setBehavior(new PushButtonBehavior(m_buttonMode,m_iNoStates));
}
ControlPushButton::~ControlPushButton() = default;
// Tell this PushButton how to act on rising and falling edges
void ControlPushButton::setButtonMode(ButtonMode mode)
{
    //qDebug() << "Setting " << m_Key.group << m_Key.item << "as toggle";
    if(m_buttonMode != mode)
    {
      m_buttonMode = mode;
      if (m_pControl) 
          m_pControl->setBehavior(new PushButtonBehavior(m_buttonMode,m_iNoStates));
      emit buttonModeChanged(mode);
    }
}
int ControlPushButton::numStates()const
{
  return m_iNoStates;
}
void ControlPushButton::setStates(int num_states)
{
    if(m_iNoStates != num_states)
    {
      m_iNoStates = num_states;
      if (m_pControl) 
              m_pControl->setBehavior(new PushButtonBehavior(m_buttonMode,m_iNoStates));
      emit numStatesChanged(num_states);
    }
}
ControlPushButton::ButtonMode ControlPushButton::getButtonMode()const
{
  return m_buttonMode;
}
