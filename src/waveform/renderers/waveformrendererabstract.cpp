#include "waveformrendererabstract.h"
#include "skin/skincontext.h"
WaveformRendererAbstract::WaveformRendererAbstract(WaveformWidgetRenderer* waveformWidgetRenderer)
        : m_waveformRenderer(waveformWidgetRenderer),
          m_dirty(true) {
}

WaveformRendererAbstract::~WaveformRendererAbstract() {
}
