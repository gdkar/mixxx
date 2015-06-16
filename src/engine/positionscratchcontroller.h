#ifndef POSITIONSCRATCHCONTROLLER_H
#define POSITIONSCRATCHCONTROLLER_H

#include <QObject>
#include <QString>

#include "engine/enginecontrol.h"
#include "engine/engineobject.h"
#include "controlobject.h"

class VelocityController;
class RateIIFilter;

class PositionScratchController : public EngineControl{
  public:
    PositionScratchController(const QString &group, ConfigObject<ConfigValue>*_config,QObject *pParent=nullptr);
    virtual ~PositionScratchController();
    
    virtual double process(const double dRate, const double currentSample, const double dTotalSamples, const int iBufferSize);
    bool isEnabled();
    double getRate();
  public slots:
    void onSeek(double currentSample);

  private:
    QString m_group;
    ControlObject* m_pScratchEnable;
    ControlObject* m_pScratchPosition;
    ControlObject* m_pMasterSampleRate;
    ControlObject* m_pPlaying;
    VelocityController* m_pVelocityController;
    RateIIFilter* m_pRateIIFilter;
    bool   m_bScratching;
    bool   m_bEnableInertia;
    double m_dLastPlaypos;
    double m_dPositionDeltaSum;
    double m_dTargetDelta;
    double m_dStartScratchPosition;
    double m_dRate;
    double m_dMoveDelay;
    double m_dMouseSampleTime;
};

#endif /* POSITIONSCRATCHCONTROLLER_H */
