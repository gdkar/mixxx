#ifndef WAVEFORMRENDERERENDOFTRACK_H
#define WAVEFORMRENDERERENDOFTRACK_H

#include <QColor>
#include <QTime>
#include <qsharedpointer.h>
#include <qatomic.h>
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
    virtual void setup(const QDomNode& node, SkinContext* context);
    virtual void onResize();
    virtual void draw(QPainter* painter, QPaintEvent* event);

  private:
    QSharedPointer<ControlObjectSlave>  m_pEndOfTrackControl;
    bool m_endOfTrackEnabled;
    QSharedPointer<ControlObjectSlave> m_pTrackSampleRate;
    QSharedPointer<ControlObjectSlave> m_pPlayControl;
    QSharedPointer<ControlObjectSlave> m_pLoopControl;

    QColor m_color;
    QTime m_timer;
    double m_remainingTimeTriggerSeconds;
    double m_blinkingPeriodMillis;

    QVector<QRect> m_backRects;
    QPen m_pen;
    //QLinearGradient m_gradient;

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererEndOfTrack);
};

#endif // WAVEFORMRENDERERENDOFTRACK_H
