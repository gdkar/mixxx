#ifndef WAVEFORMRENDERMARK_H
#define WAVEFORMRENDERMARK_H

#include "skin/skincontext.h"
#include "util/class.h"
#include "waveform/renderers/waveformmarkset.h"
#include "waveform/renderers/waveformrendererabstract.h"

class WaveformRenderMark : public WaveformRendererAbstract {
    Q_OBJECT
  public:
    Q_INVOKABLE explicit WaveformRenderMark(WaveformWidgetRenderer* waveformWidgetRenderer);

    virtual void setup(const QDomNode& node, const SkinContext& context);
    virtual void draw(QPainter* painter, QPaintEvent* event);

  private:
    void generateMarkImage(WaveformMark& mark);

    WaveformMarkSet m_marks;
    DISALLOW_COPY_AND_ASSIGN(WaveformRenderMark);
};

#endif
