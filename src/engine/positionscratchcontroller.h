#ifndef POSITIONSCRATCHCONTROLLER_H
#define POSITIONSCRATCHCONTROLLER_H

#include <QObject>
#include <QString>

#include "control/controlobject.h"

class VelocityController;
class RateIIFilter;

class PositionScratchController : public QObject {
    Q_OBJECT
    Q_PROPERTY(double rate READ getRate WRITE setRate NOTIFY rateChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged);
  public:
    PositionScratchController(QString group);
    virtual ~PositionScratchController();

    void process(double currentSample, double releaseRate,
                 int iBufferSize, double baserate);
    bool   isEnabled() const;
    void   setEnabled(bool);
    double getRate() const;
    void   setRate(double);
  public slots:
    void notifySeek(double currentSample);
  signals:
    void rateChanged(double);
    void enabledChanged(bool);
  private:
    const QString m_group;
    ControlObject* m_pScratchEnable;
    ControlObject* m_pScratchPosition;
    ControlObject* m_pMasterSampleRate;
    VelocityController* m_pVelocityController;
    RateIIFilter* m_pRateIIFilter;
    bool m_bScratching;
    bool m_bEnableInertia;
    double m_dLastPlaypos;
    double m_dPositionDeltaSum;
    double m_dTargetDelta;
    double m_dStartScratchPosition;
    double m_dRate;
    double m_dMoveDelay;
    double m_dMouseSampeTime;
};

#endif /* POSITIONSCRATCHCONTROLLER_H */
