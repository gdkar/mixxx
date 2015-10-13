#include "qtsimplewaveformwidget.h"

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
        : WaveformWidgetAbstract(group) {
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
QtSimpleWaveformWidget::~QtSimpleWaveformWidget() = default;
void QtSimpleWaveformWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    draw(&painter, e);
}
