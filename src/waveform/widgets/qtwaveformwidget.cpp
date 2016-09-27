#include <QPainter>
#include <QtDebug>

#include "waveform/widgets/qtwaveformwidget.h"

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/renderers/waveformrenderbackground.h"
#include "waveform/renderers/qtwaveformrendererfilteredsignal.h"
#include "waveform/renderers/waveformrendererpreroll.h"
#include "waveform/renderers/waveformrendermark.h"
#include "waveform/renderers/waveformrendermarkrange.h"
#include "waveform/renderers/waveformrendererendoftrack.h"
#include "waveform/renderers/waveformrenderbeat.h"

#include "util/performancetimer.h"

QtWaveformWidget::QtWaveformWidget(const char* group, QWidget* parent)
        : QWidget(parent),
          WaveformWidgetAbstract(group) {
    addRenderer<WaveformRenderBackground>();
    addRenderer<WaveformRendererEndOfTrack>();
    addRenderer<WaveformRendererPreroll>();
    addRenderer<WaveformRenderMarkRange>();
    addRenderer<QtWaveformRendererFilteredSignal>();
    addRenderer<WaveformRenderBeat>();
    addRenderer<WaveformRenderMark>();

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);

    m_initSuccess = init();
}

QtWaveformWidget::~QtWaveformWidget() {
}

void QtWaveformWidget::castToQWidget() {
    m_widget = qobject_cast<QWidget*>(this);
}

void QtWaveformWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    draw(&painter, event);
}
