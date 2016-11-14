#include <QPainter>
//#include <QGLContext>
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
    addRenderer(std::make_unique<WaveformRenderBackground>(this));
    addRenderer(std::make_unique<WaveformRendererEndOfTrack>(this));
    addRenderer(std::make_unique<WaveformRendererPreroll>(this));
    addRenderer(std::make_unique<WaveformRenderMarkRange>(this));
    addRenderer(std::make_unique<QtWaveformRendererFilteredSignal>(this));
    addRenderer(std::make_unique<WaveformRenderBeat>(this));
    addRenderer(std::make_unique<WaveformRenderMark>(this));

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);

//    setAutoBufferSwap(false);

    m_initSuccess = init();
}

QtWaveformWidget::~QtWaveformWidget() = default;

void QtWaveformWidget::castToQWidget() {
    m_widget = static_cast<QWidget*>(this);
}
void QtWaveformWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    draw(&painter, event);
}
