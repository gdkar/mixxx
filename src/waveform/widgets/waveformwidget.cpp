#include "waveformwidget.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/renderers/waveformrenderbackground.h"
#include "waveform/renderers/qtwaveformrendererfilteredsignal.h"
#include "waveform/renderers/qtwaveformrenderersimplesignal.h"
#include "waveform/renderers/waveformrendererfilteredsignal.h"
#include "waveform/renderers/waveformrendererrgb.h"
#include "waveform/renderers/waveformrendererhsv.h"
#include "waveform/renderers/waveformrendererpreroll.h"
#include "waveform/renderers/waveformrendermark.h"
#include "waveform/renderers/waveformrendermarkrange.h"
#include "waveform/renderers/waveformrendererendoftrack.h"
#include "waveform/renderers/waveformrenderbeat.h"


#include <QtDebug>
#include <QWidget>


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
void WaveformWidget::hold()
{
    hide();
}
void WaveformWidget::release()
{
    show();
}
int WaveformWidget::render()
{
    repaint();
    return 0; // Time for Painter setup, unknown in this case
}
void WaveformWidget::resize(int width, int height)
{
    QWidget::resize(width,height);
    WaveformWidgetRenderer::resize(width, height);
}
void WaveformWidget::setRenderType(WaveformWidgetType::Type t)
{
  if ( t != m_type )
  {
    delete std::exchange(m_rendererStack[4],nullptr);
    switch ( t )
    {
      case WaveformWidgetType::SoftwareWaveform:
        m_rendererStack[4] = new WaveformRendererFilteredSignal(this);
        break;
      case WaveformWidgetType::QtSimpleWaveform:
        m_rendererStack[4] = new QtWaveformRendererSimpleSignal(this);
        break;
      case WaveformWidgetType::QtWaveform:
        m_rendererStack[4] = new QtWaveformRendererFilteredSignal(this);
        break;
      case WaveformWidgetType::HSVWaveform:
        m_rendererStack[4] = new WaveformRendererHSV(this);
        break;
      case WaveformWidgetType::RGBWaveform:
        m_rendererStack[4] = new WaveformRendererRGB(this);
        break;
      default:
        qDebug() << "Invalid Waveform Type";
        break;
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
WaveformWidgetType::Type WaveformWidget::getType() const
{
  return m_type;
}
