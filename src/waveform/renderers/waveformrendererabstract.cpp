#include "waveformrendererabstract.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

WaveformRendererAbstract::WaveformRendererAbstract(WaveformWidgetRenderer* waveformWidgetRenderer)
        : QObject(waveformWidgetRenderer),
          m_waveformRenderer(waveformWidgetRenderer),
          m_dirty(true)
{ }
WaveformRendererAbstract::~WaveformRendererAbstract() = default;
