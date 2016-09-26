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

#ifndef CONTROLPOTMETER_H
#define CONTROLPOTMETER_H

#include "preferences/usersettings.h"
#include "control/controlobject.h"

/**
  *@author Tue and Ken Haste Andersen
  */

class ControlPushButton;
class ControlProxy;

/*class PotmeterControls : public QObject {
    Q_OBJECT
  public:
    PotmeterControls(const ConfigKey& key);
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
    // Sets the control to its default
    void setToDefault(double);
    // Toggles the value between 0.0 and 1.0.
    void toggleValue(double);
    // Toggles the value between -1.0 and 0.0.
  private:
    ControlProxy* m_pControl;
    int m_stepCount;
    double m_smallStepCount;
};*/

class ControlPotmeter : public ControlObject {
    Q_OBJECT
    Q_PROPERTY(double min READ minValue WRITE setMinValue NOTIFY minValueChanged);
    Q_PROPERTY(double max READ maxValue WRITE setMaxValue NOTIFY maxValueChanged);
    Q_PROPERTY(bool unconstrain READ unconstrain WRITE setUnconstrain NOTIFY unconstrainChanged);
    Q_PROPERTY(int stepCount READ stepCount WRITE setStepCount NOTIFY stepCountChanged);
    Q_PROPERTY(int smallStepCount READ smallStepCount WRITE setSmallStepCount NOTIFY smallStepCountChanged);
  public:
    ControlPotmeter(ConfigKey key, double dMinValue = 0.0, double dMaxValue = 1.0,
                    bool allowOutOfBounds = false,
                    bool bIgnoreNops = true,
                    bool bTrack = false,
                    bool bPersist = false);

    ControlPotmeter(ConfigKey key, QObject *p, double dMinValue = 0.0, double dMaxValue = 1.0,
                    bool allowOutOfBounds = false,
                    bool bIgnoreNops = true,
                    bool bTrack = false,
                    bool bPersist = false);
    virtual ~ControlPotmeter();
    // Sets the step count of the associated PushButtons.
    void setStepCount(int count);
    int stepCount() const;
    // Sets the small step count of the associated PushButtons.
    void setSmallStepCount(int count);
    int smallStepCount() const;
    // Sets the minimum and maximum allowed value. The control value is reset
    // when calling this method
    void setRange(double dMinValue, double dMaxValue, bool allowOutOfBounds);
    double minValue() const;
    double maxValue() const;
    bool   unconstrain() const;
    void setMinValue(double);
    void setMaxValue(double);
    void setUnconstrain(bool);
  signals:
    void unconstrainChanged(bool);
    void minValueChanged(double);
    void maxValueChanged(double);
    void stepCountChanged(int);
    void smallStepCountChanged(int);
  protected:
    bool m_bAllowOutOfBounds;
//    PotmeterControls m_controls;
    int m_stepCount;
    double m_smallStepCount;
    double m_min;
    double m_max;
};

#endif
