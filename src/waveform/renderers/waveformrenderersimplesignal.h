#ifndef WAVEFORMRENDERERSIMPLESIGNAL_H
#define WAVEFORMRENDERERSIMPLESIGNAL_H

#include "waveformrenderersignalbase.h"
#include "waveform/waveform.h"
#include "waveform/path_simplify.h"
#include <vector>
#include <QLineF>

#include "util.h"

class WaveformRendererSimpleSignal : public WaveformRendererSignalBase {
  public:
    explicit WaveformRendererSimpleSignal(
        WaveformWidgetRenderer* waveformWidget);
    virtual ~WaveformRendererSimpleSignal();

    virtual void onSetup(const QDomNode& node);

    virtual void draw(QPainter* painter, QPaintEvent* event);

    virtual void onResize();

  private:
    QVector<QPointF>      m_upper;
    QVector<QPointF>      m_lower;
    QVector<QPointF>          m_path;
    ConstWaveformPointer  m_wf;
    int               m_prev_size;
    DISALLOW_COPY_AND_ASSIGN(WaveformRendererSimpleSignal);
};

#endif // WAVEFORMRENDERERSIMPLESIGNAL_H
