#include "waveformwidget.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/renderers/waveformrenderbackground.h"
#include "waveform/renderers/qtwaveformrendererfilteredsignal.h"
#include "waveform/renderers/qtwaveformrenderersimplesignal.h"
#include "waveform/renderers/waveformrendererrgb.h"
#include "waveform/renderers/waveformrendererhsv.h"
#include "waveform/renderers/waveformrendererpreroll.h"
#include "waveform/renderers/waveformrendermark.h"
#include "waveform/renderers/waveformrendermarkrange.h"
#include "waveform/renderers/waveformrendererendoftrack.h"
#include "waveform/renderers/waveformrenderbeat.h"


#include <QtDebug>
#include <QWidget>
#include <QMetaEnum>

WaveformWidget::WaveformWidget(const char* group,QWidget *p)
    : QWidget(p),
    WaveformWidgetRenderer(group)
{
  m_rendererStack.push_back(new WaveformRenderBackground(this));
  m_rendererStack.push_back(new WaveformRendererEndOfTrack(this));
  m_rendererStack.push_back(new WaveformRendererPreroll(this));
  m_rendererStack.push_back(new WaveformRenderMarkRange(this));
  m_rendererStack.push_back(nullptr);
  m_rendererStack.push_back(new WaveformRenderBeat(this));
  m_rendererStack.push_back(new WaveformRenderMark(this));
  m_initSuccess = init ();
}
WaveformWidget::~WaveformWidget() = default;
int WaveformWidget::render()
{
    update();
    return 0; // Time for Painter setup, unknown in this case
}
void WaveformWidget::onPreRender(int i)
{
  WaveformWidgetRenderer::onPreRender(i);
}
void WaveformWidget::resize(int width, int height)
{
    QWidget::resize(width,height);
    WaveformWidgetRenderer::resize(width, height);
}
void WaveformWidget::setRenderType(RenderType t)
{
  if ( t != m_type )
  {
    delete std::exchange(m_rendererStack[4],nullptr);
    switch ( t )
    {
      case Simple:
        m_rendererStack[4] = new QtWaveformRendererSimpleSignal(this);
        break;
      case Filtered:
        m_rendererStack[4] = new QtWaveformRendererFilteredSignal(this);
        break;
      case HSV:
        m_rendererStack[4] = new WaveformRendererHSV(this);
        break;
      case RGB:
        m_rendererStack[4] = new WaveformRendererRGB(this);
        break;
      default:
        qDebug() << "Invalid Waveform Type";
      case Empty: break;
    }
    m_type = t;
    if ( m_initSuccess ) m_initSuccess = init ();
  }
}
void WaveformWidget::paintEvent(QPaintEvent *e)
{
  QPainter painter(this);
  draw(&painter,e);
}
bool WaveformWidget::isValid() const
{
  return m_initSuccess;
}
WaveformWidget::RenderType WaveformWidget::getType() const
{
  return m_type;
}
