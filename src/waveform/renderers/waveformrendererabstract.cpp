#include "waveformrendererabstract.h"

WaveformRendererAbstract::WaveformRendererAbstract(WaveformWidgetRenderer* waveformWidgetRenderer)
        : m_waveformRenderer(waveformWidgetRenderer)
{
}
WaveformRendererAbstract::~WaveformRendererAbstract() = default;

bool WaveformRendererAbstract::init()
{
  return true;
}
void WaveformRendererAbstract::onResize()
{
}
void WaveformRendererAbstract::onSetTrack()
{
}
bool WaveformRendererAbstract::isDirty() const
{
  return m_dirty;
}
void WaveformRendererAbstract::setDirty(bool d)
{
  m_dirty = d;
}

