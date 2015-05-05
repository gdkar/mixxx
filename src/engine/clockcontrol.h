#ifndef CLOCKCONTROL_H
#define CLOCKCONTROL_H

#include "configobject.h"
#include "engine/enginecontrol.h"

#include "track/trackinfoobject.h"
#include "track/beats.h"

class ControlObjectSlave;

class ClockControl: public EngineControl {
    Q_OBJECT
  public:
    ClockControl(QString group,ConfigObject<ConfigValue>* pConfig, QObject *pParent=0);
    virtual ~ClockControl();
    double process(const double dRate, const double currentSample,
                   const double totalSamples, const int iBufferSize);
  public slots:
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);
    void onBeatsUpdated();
  private:
    ControlObject* m_pCOBeatActive;
    ControlObjectSlave* m_pCOSampleRate;
    TrackPointer m_pTrack;
    BeatsPointer m_pBeats;
};

#endif /* CLOCKCONTROL_H */
