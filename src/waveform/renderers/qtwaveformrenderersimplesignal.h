_Pragma("once")
#include "waveformrenderersignalbase.h"
#include <QBrush>
#include <QPen>
#include <vector>

class ControlObject;

class QtWaveformRendererSimpleSignal : public WaveformRendererSignalBase {
public:
    explicit QtWaveformRendererSimpleSignal(WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~QtWaveformRendererSimpleSignal();
    virtual void onSetup(const QDomNode &node);
    virtual void draw(QPainter* painter, QPaintEvent* event);
protected:
    virtual void onResize();
private:
    int buildPolygon();
    QBrush m_allBrush;
    QPen m_allPen;
    std::vector<QPoint> m_polygon;
};
