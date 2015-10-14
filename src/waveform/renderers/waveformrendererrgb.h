_Pragma("once")
#include "waveformrenderersignalbase.h"
#include "util.h"

class WaveformRendererRGB : public WaveformRendererSignalBase
{
  public:
    explicit WaveformRendererRGB(WaveformWidgetRenderer* waveformWidget);
    virtual ~WaveformRendererRGB();

    virtual void onSetup(const QDomNode& node);
    virtual void draw(QPainter* painter, QPaintEvent* event);
};
