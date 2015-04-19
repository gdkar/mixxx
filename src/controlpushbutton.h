/***************************************************************************
                          controlpushbutton.h  -  description
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

#ifndef CONTROLPUSHBUTTON_H
#define CONTROLPUSHBUTTON_H

#include "controlobject.h"
#include "control/controlbehavior.h"
/**
  *@author Tue and Ken Haste Andersen
  */

class ControlPushButton : public ControlObject {
    Q_OBJECT
  public:
    typedef ControlPushButtonBehavior::ButtonMode ButtonMode;

    static QString buttonModeToString(int mode) {
        switch(mode) {
            case ControlPushButtonBehavior::PUSH:
                return "PUSH";
            case ControlPushButtonBehavior::TOGGLE:
                return "TOGGLE";
            case ControlPushButtonBehavior::POWERWINDOW:
                return "POWERWINDOW";
            case ControlPushButtonBehavior::LONGPRESSLATCHING:
                return "LONGPRESSLATCHING";
            case ControlPushButtonBehavior::TRIGGER:
                return "TRIGGER";
            default:
                return "UNKNOWN";
        }
    }

    ControlPushButton(ConfigKey key, bool bPersist=false);
    virtual ~ControlPushButton();

    inline ButtonMode getButtonMode() const {
        return m_buttonMode;
    }
    void setButtonMode(ButtonMode mode);
    void setStates(int num_states);

  private:
    ButtonMode m_buttonMode;
    int m_iNoStates;
};

#endif
