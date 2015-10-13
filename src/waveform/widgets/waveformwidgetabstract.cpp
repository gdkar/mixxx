#include "waveformwidgetabstract.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

#include <QtDebug>
#include <QWidget>


WaveformWidgetAbstract::WaveformWidgetAbstract(const char* group,QWidget *p)
    : QWidget(p),
    WaveformWidgetRenderer(group)
{
}
WaveformWidgetAbstract::~WaveformWidgetAbstract() = default;
void WaveformWidgetAbstract::hold()
{
    hide();
}
void WaveformWidgetAbstract::release()
{
    show();
}
void WaveformWidgetAbstract::preRender(VSyncThread* vsyncThread)
{
    WaveformWidgetRenderer::onPreRender(vsyncThread);
}
int WaveformWidgetAbstract::render()
{
    repaint();
    return 0; // Time for Painter setup, unknown in this case
}
void WaveformWidgetAbstract::resize(int width, int height)
{
    QWidget::resize(width,height);
    WaveformWidgetRenderer::resize(width, height);
}
void WaveformWidgetAbstract::paintEvent(QPaintEvent *e)
{
  QPainter painter(this);
  draw(&painter,e);
}
