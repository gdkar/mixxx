_Pragma("once")
#include <QColor>

#include "util.h"
#include "waveform/renderers/waveformrendererabstract.h"
#include "skin/skincontext.h"

class WaveformRendererPreroll : public WaveformRendererAbstract {
  public:
    explicit WaveformRendererPreroll(WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~WaveformRendererPreroll();
    virtual void setup(const QDomNode& node, const SkinContext& context);
    virtual void draw(QPainter* painter, QPaintEvent* event);
  private:
    QColor m_color;
};
