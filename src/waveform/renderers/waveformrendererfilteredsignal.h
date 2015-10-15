_Pragma("once")
#include "waveformrenderersignalbase.h"

#include <vector>
#include <QLineF>

#include "util.h"

class WaveformRendererFilteredSignal : public WaveformRendererSignalBase
{
  public:
    explicit WaveformRendererFilteredSignal( WaveformWidgetRenderer* waveformWidget);
    virtual ~WaveformRendererFilteredSignal();
    virtual void onSetup(const QDomNode& node);
    virtual void draw(QPainter* painter, QPaintEvent* event);
    virtual void onResize();
  private:
    std::vector<QLine> m_allLines;
    std::vector<QLine> m_lowLines;
    std::vector<QLine> m_midLines;
    std::vector<QLine> m_highLines;
};
