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
_Pragma("once")
#include "controlobject.h"

/**
  *@author Tue and Ken Haste Andersen
  */

class ControlPushButton : public ControlObject {
    Q_OBJECT
  public:
    enum ButtonMode {
         PUSH = 0,
         TOGGLE,
         POWERWINDOW,
         LONGPRESSLATCHING,
         TRIGGER,
    };
    Q_ENUM(ButtonMode);
    Q_PROPERTY(ButtonMode buttonMode READ getButtonMode WRITE setButtonMode NOTIFY buttonModeChanged);
    Q_PROPERTY(int numStates READ numStates WRITE setStates NOTIFY numStatesChanged);
    static QString buttonModeToString(int mode);
    ControlPushButton(ConfigKey key, bool bPersist=false,QObject *pParent=nullptr);
    virtual ~ControlPushButton();
   ButtonMode getButtonMode() const;
    void setButtonMode(enum ButtonMode mode);
    int  numStates()const;
    void setStates(int num_states);
  signals:
    void buttonModeChanged(ButtonMode);
    void numStatesChanged(int);
  private:
    enum ButtonMode m_buttonMode;
    int m_iNoStates;
};
