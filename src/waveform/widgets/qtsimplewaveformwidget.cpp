#include "waveform/widgets/qtsimplewaveformwidget.h"

#include <QPainter>
#include <QtDebug>

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/renderers/waveformrenderbackground.h"
#include "waveform/renderers/qtwaveformrenderersimplesignal.h"
#include "waveform/renderers/waveformrendererpreroll.h"
#include "waveform/renderers/waveformrendermark.h"
#include "waveform/renderers/waveformrendermarkrange.h"
#include "waveform/renderers/waveformrendererendoftrack.h"
#include "waveform/renderers/waveformrenderbeat.h"

#include "util/performancetimer.h"

QtSimpleWaveformWidget::QtSimpleWaveformWidget(const char* group, QWidget* parent)
        : QWidget(parent),
          WaveformWidgetAbstract(group) {
    addRenderer<WaveformRenderBackground>();
    addRenderer<WaveformRendererEndOfTrack>();
    addRenderer<WaveformRendererPreroll>();
    addRenderer<WaveformRenderMarkRange>();
    addRenderer<QtWaveformRendererSimpleSignal>();
    addRenderer<WaveformRenderBeat>();
    addRenderer<WaveformRenderMark>();

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);

    m_initSuccess = init();
}

QtSimpleWaveformWidget::~QtSimpleWaveformWidget() { }

void QtSimpleWaveformWidget::castToQWidget()
{
    m_widget = qobject_cast<QWidget*>(this);
}
void QtSimpleWaveformWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    draw(&painter, event);
}
