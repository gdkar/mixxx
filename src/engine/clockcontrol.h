_Pragma("once")
#include "configobject.h"
#include "engine/enginecontrol.h"

#include "trackinfoobject.h"
#include "track/beats.h"

class ControlObjectSlave;
class ControlObject;
class ClockControl: public EngineControl {
    Q_OBJECT
  public:
    ClockControl(QString group, ConfigObject<ConfigValue>* pConfig, QObject *p);
    virtual ~ClockControl();
    double process(double dRate, double currentSample, double totalSamples, int iBufferSize);
  public slots:
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);
    void slotBeatsUpdated();
  private:
    ControlObject* m_pCOBeatActive;
    ControlObjectSlave* m_pCOSampleRate;
    TrackPointer m_pTrack;
    BeatsPointer m_pBeats;
};
