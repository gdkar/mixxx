_Pragma("once")
#include <QObject>

#include "configobject.h"
#include "engine/enginecontrol.h"

#include "trackinfoobject.h"
#include "track/beats.h"

class ControlObject;
class ControlPushButton;

class QuantizeControl : public EngineControl {
    Q_OBJECT
  public:
    QuantizeControl(QString group, ConfigObject<ConfigValue>* pConfig,QObject*);
    virtual ~QuantizeControl();
    virtual void setCurrentSample(double dCurrentSample, double dTotalSamples);
  public slots:
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);
  private slots:
    void slotBeatsUpdated();
  private:
    // Update positions of previous and next beats from beatgrid.
    void lookupBeatPositions(double dCurrentSample);
    // Update position of the closest beat based on existing previous and
    // next beat values.  Usually callers will call lookupBeatPositions first.
    void updateClosestBeat(double dCurrentSample);
    ControlPushButton* m_pCOQuantizeEnabled = nullptr;
    ControlObject* m_pCONextBeat    = nullptr;
    ControlObject* m_pCOPrevBeat    = nullptr;
    ControlObject* m_pCOClosestBeat = nullptr;
    TrackPointer m_pTrack{nullptr};
    BeatsPointer m_pBeats{nullptr};
};
