/***************************************************************************
                          controlpotmeter.h  -  description
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
#include "configobject.h"
#include "controlobject.h"

/**
  *@author Tue and Ken Haste Andersen
  */

class ControlObject;

class PotmeterControls : public QObject {
    Q_OBJECT
  public:
    PotmeterControls(const ConfigKey& key,QObject *pParent=nullptr);
    virtual ~PotmeterControls();
    void setStepCount(int count);
    void setSmallStepCount(int count);
  public slots:
    // Increases the value.
    void incValue(double);
    // Decreases the value.
    void decValue(double);
    // Increases the value by smaller step.
    void incSmallValue(double);
    // Decreases the value by smaller step.
    void decSmallValue(double);
    // Sets the value to 1.0.
    void setToOne(double);
    // Sets the value to -1.0.
    void setToMinusOne(double);
    // Sets the value to 0.0.
    void setToZero(double);
    // Sets the control to its default
    void setToDefault(double);
    // Toggles the value between 0.0 and 1.0.
    void toggleValue(double);
    // Toggles the value between -1.0 and 0.0.
    void toggleMinusValue(double);
  private:
    ControlObject* m_pControl = nullptr;
    int m_stepCount = 0;
    int m_smallStepCount = 0;
};
class ControlPotmeter : public ControlObject {
    Q_OBJECT
    Q_PROPERTY(int stepCount READ stepCount WRITE setStepCount NOTIFY stepCountChanged);
    Q_PROPERTY(int smallStepCount READ smallStepCount WRITE setSmallStepCount NOTIFY smallStepCountChanged);
    Q_PROPERTY(double minValue READ minValue WRITE setMinValue NOTIFY rangeChanged);
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue NOTIFY rangeChanged);
  public:
    ControlPotmeter(ConfigKey key, double dMinValue = 0.0, double dMaxValue = 1.0,
                    bool bAllowOutOfBounds = false,
                    bool bIgnoreNops = true,
                    bool bTrack = false,
                    bool bPersist = false);
    virtual ~ControlPotmeter();
    // Sets the step count of the associated PushButtons.
    void setStepCount(int count);
    // Sets the small step count of the associated PushButtons.
    void setSmallStepCount(int count);
    int  stepCount()const;
    int  smallStepCount()const;
    double minValue()const;
    double maxValue()const;
    bool   allowOutOfBounds()const;
    void   setMinValue(double);
    void   setMaxValue(double);
    void   setAllowOutOfBounds(bool);
    // Sets the minimum and maximum allowed value. The control value is reset
    // when calling this method
    void setRange(double dMinValue, double dMaxValue, bool allowOutOfBounds);
  signals:
    void rangeChanged();
    void smallStepCountChanged(int);
    void stepCountChanged(int);
  protected:
    bool m_bAllowOutOfBounds;
    double m_minValue;
    double m_maxValue;
    int m_stepCount;
    int m_smallStepCount;
    PotmeterControls m_controls;
};
