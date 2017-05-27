#include "rgbwaveformwidget.h"

#include <QPainter>

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/renderers/waveformrenderbackground.h"
#include "waveform/renderers/waveformrendermark.h"
#include "waveform/renderers/waveformrendermarkrange.h"
#include "waveform/renderers/waveformrendererrgb.h"
#include "waveform/renderers/waveformrendererpreroll.h"
#include "waveform/renderers/waveformrendererendoftrack.h"
#include "waveform/renderers/waveformrenderbeat.h"

RGBWaveformWidget::RGBWaveformWidget(const char* group, QWidget* parent)
        : QWidget(parent),
          WaveformWidgetAbstract(group) {
    addRenderer(std::make_unique<WaveformRenderBackground>(this));
    addRenderer(std::make_unique<WaveformRendererEndOfTrack>(this));
    addRenderer(std::make_unique<WaveformRendererPreroll>(this));
    addRenderer(std::make_unique<WaveformRenderMarkRange>(this));
    addRenderer(std::make_unique<WaveformRendererRGB>(this));
    addRenderer(std::make_unique<WaveformRenderBeat>(this));
    addRenderer(std::make_unique<WaveformRenderMark>(this));

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);

    m_initSuccess = init();
}

RGBWaveformWidget::~RGBWaveformWidget() {
}

void RGBWaveformWidget::castToQWidget() {
    m_widget = static_cast<QWidget*>(this);
}

void RGBWaveformWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    draw(&painter,event);
}
