#include <QPainter>

#include "waveform/widgets/emptywaveformwidget.h"

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/renderers/waveformrenderbackground.h"

EmptyWaveformWidget::EmptyWaveformWidget(const char* group, QWidget* parent)
        :WaveformWidgetRenderer(group, parent) {
    //Empty means just a background ;)
    addRenderer<WaveformRenderBackground>();
    m_initSuccess = init();
}
EmptyWaveformWidget::~EmptyWaveformWidget() = default;
mixxx::Duration EmptyWaveformWidget::render() {
    // skip update every frame
    return mixxx::Duration();
}
