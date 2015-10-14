_Pragma("once")
#include <QColor>
#include <QTime>
//#include <QLinearGradient>

#include "util.h"
#include "waveformrendererabstract.h"
#include "skin/skincontext.h"
#include "waveform/waveformwidgetfactory.h"

class ControlObject;
class ControlObjectSlave;

class WaveformRendererEndOfTrack : public WaveformRendererAbstract {
  public:
    static const int s_maxAlpha = 125;
    explicit WaveformRendererEndOfTrack(
            WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~WaveformRendererEndOfTrack();

    virtual bool init();
    virtual void setup(const QDomNode& node, const SkinContext& context);
    virtual void onResize();
    virtual void draw(QPainter* painter, QPaintEvent* event);

  private:
    ControlObjectSlave*  m_pEndOfTrackControl = nullptr;
    bool m_endOfTrackEnabled = false;
    ControlObjectSlave* m_pTrackSampleRate = nullptr;
    ControlObjectSlave* m_pPlayControl = nullptr;
    ControlObjectSlave* m_pLoopControl = nullptr;
    QColor m_color;
    QTime m_timer;
    int m_remainingTimeTriggerSeconds = 30;
    int m_blinkingPeriodMillis = 200;
    QVector<QRect> m_backRects;
    QPen m_pen;
    //QLinearGradient m_gradient;
};
