/***************************************************************************
                          enginevumeter.h  -  description
                             -------------------
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

#ifndef ENGINEVUMETER_H
#define ENGINEVUMETER_H

#include "engine/engineobject.h"

// Rate at which the vumeter is updated (using a sample rate of 44100 Hz):
#define VU_UPDATE_RATE 30 // in 1/s, fits to display frame rate
#define PEAK_DURATION 500 // in ms

// SMOOTHING FACTORS
// Must be from 0-1 the lower the factor, the more smoothing that is applied
#define ATTACK_SMOOTHING 1. // .85
#define DECAY_SMOOTHING .1  //.16//.4

class ControlObject;
class ControlProxy;

class EngineVuMeter : public EngineObject {
    Q_OBJECT
  public:
    EngineVuMeter(QObject *p,QString group);
    virtual ~EngineVuMeter();

    virtual void process(CSAMPLE* pInOut, int iBufferSize) override;
    virtual void collectFeatures(GroupFeatureState* pGroupFeatures) const override;
    void reset();
  private:
    void doSmooth(CSAMPLE &currentVolume, CSAMPLE newVolume);
    ControlObject * m_ctrlVuMeter;
    ControlObject * m_ctrlVuMeterL;
    ControlObject * m_ctrlVuMeterR;
    CSAMPLE m_fRMSvolumeL;
    CSAMPLE m_fRMSvolumeSumL;
    CSAMPLE m_fRMSvolumeR;
    CSAMPLE m_fRMSvolumeSumR;
    int m_iSamplesCalculated;

    ControlObject * m_ctrlPeakIndicator;
    ControlObject * m_ctrlPeakIndicatorL;
    ControlObject * m_ctrlPeakIndicatorR;
    int m_peakDurationL;
    int m_peakDurationR;

    ControlProxy* m_pSampleRate;
};

#endif
