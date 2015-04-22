#ifndef WAVEFORMRENDERBEAT_H
#define WAVEFORMRENDERBEAT_H

#include <QColor>
#include <qsharedpointer.h>
#include <qatomic.h>
#include "waveform/renderers/waveformrendererabstract.h"
#include "util.h"

class SkinContext;
class ControlObjectSlave;

class WaveformRenderBeat : public WaveformRendererAbstract {
  public:
    explicit WaveformRenderBeat(WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~WaveformRenderBeat();

    virtual bool init();
    virtual void setup(const QDomNode& node, SkinContext *context);
    virtual void draw(QPainter* painter, QPaintEvent* event);

  private:
    QColor m_beatColor;
    QSharedPointer<ControlObjectSlave> m_pBeatActive;
    QVector<QLineF> m_beats;
    DISALLOW_COPY_AND_ASSIGN(WaveformRenderBeat);
};

#endif //WAVEFORMRENDERBEAT_H
