#ifndef WAVEFORMRENDERERSIMPLESIGNAL_H
#define WAVEFORMRENDERERSIMPLESIGNAL_H

#include "waveformrenderersignalbase.h"

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
    std::vector<QLineF> m_lowLines;
    std::vector<QLineF> m_midLines;
    std::vector<QLineF> m_highLines;

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererSimpleSignal);
};

#endif // WAVEFORMRENDERERILTEREDSIGNAL_H
