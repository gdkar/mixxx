_Pragma("once")
#include <QColor>
#include <vector>
#include "waveform/renderers/waveformrendererabstract.h"
#include "util.h"
#include "skin/skincontext.h"

class ControlObjectSlave;

class WaveformRenderBeat : public WaveformRendererAbstract {
  public:
    explicit WaveformRenderBeat(WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~WaveformRenderBeat();

    virtual bool init();
    virtual void setup(const QDomNode& node, const SkinContext& context);
    virtual void draw(QPainter* painter, QPaintEvent* event);

  private:
    QColor m_beatColor;
    ControlObjectSlave* m_pBeatActive = nullptr;
    std::vector<QLineF> m_beats;

};
