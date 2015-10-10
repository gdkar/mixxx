_Pragma("once")
#include <QObject>
#include <QString>
#include <memory>
class ControlObject;
class VelocityController;
class RateIIFilter;

class PositionScratchController : public QObject {
  Q_OBJECT
  public:
    PositionScratchController(QString group,QObject *p);
    virtual ~PositionScratchController();

    void process(double currentSample, double releaseRate,
                 int iBufferSize, double baserate);
    bool isEnabled();
    double getRate();
    void notifySeek(double currentSample);

  private:
    const QString m_group;
    ControlObject* m_pScratchEnable    = nullptr;
    ControlObject* m_pScratchPosition  = nullptr;
    ControlObject* m_pMasterSampleRate = nullptr;
    std::unique_ptr<VelocityController> m_pVelocityController;
    std::unique_ptr<RateIIFilter> m_pRateIIFilter;
    bool m_bScratching              = false;
    bool m_bEnableInertia           = false;
    double m_dLastPlaypos           = 0;
    double m_dPositionDeltaSum      = 0;
    double m_dTargetDelta           = 0;
    double m_dStartScratchPosition  = 0;
    double m_dRate                  = 0;
    double m_dMoveDelay             = 0;
    double m_dMouseSampleTime       = 0;
};
