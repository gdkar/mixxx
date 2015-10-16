#include <QDomNode>
#include <QPaintEvent>
#include <QPainter>

#include "waveform/renderers/waveformrenderbeat.h"

#include "controlobject.h"
#include "controlobjectslave.h"
#include "track/beats.h"
#include "trackinfoobject.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "widget/wskincolor.h"
#include "widget/wwidget.h"

WaveformRenderBeat::WaveformRenderBeat(WaveformWidgetRenderer* waveformWidgetRenderer)
        : WaveformRendererAbstract(waveformWidgetRenderer),
          m_pBeatActive(nullptr)
{
    m_beats.reserve(128);
}

WaveformRenderBeat::~WaveformRenderBeat()
{
    if (m_pBeatActive) delete m_pBeatActive;
}
bool WaveformRenderBeat::init()
{
    if(!m_pBeatActive) m_pBeatActive = new ControlObjectSlave(m_waveformRenderer->getGroup(), "beat_active");
    return m_pBeatActive->valid();
}
void WaveformRenderBeat::setup(const QDomNode& node, const SkinContext& context)
{
    m_beatColor.setNamedColor(context.selectString(node, "BeatColor"));
    m_beatColor = WSkinColor::getCorrectColor(m_beatColor).toRgb();
    m_beatColor.setAlphaF(1.0);
}
void WaveformRenderBeat::draw(QPainter* painter, QPaintEvent* /*event*/)
{
    auto trackInfo = m_waveformRenderer->getTrackInfo();
    if (!trackInfo)return;
    auto trackBeats = trackInfo->getBeats();
    if (!trackBeats) return;
    auto trackSamples = m_waveformRenderer->getTrackSamples();
    if (trackSamples <= 0) return;
    auto firstDisplayedPosition = m_waveformRenderer->getFirstDisplayedPosition();
    auto lastDisplayedPosition = m_waveformRenderer->getLastDisplayedPosition();
    auto it = trackBeats->findBeats(firstDisplayedPosition * trackSamples, lastDisplayedPosition * trackSamples);
    // if no beat do not waste time saving/restoring painter
    if (!it.hasNext()) return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    auto beatPen = QPen(m_beatColor);
    beatPen.setWidthF(1);
    painter->setPen(beatPen);
    auto  rendererHeight = m_waveformRenderer->getHeight();
    m_beats.clear();
    while (it.hasNext())
    {
        auto beatPosition = it.next();
        auto xBeatPoint = m_waveformRenderer->transformSampleIndexInRendererWorld(beatPosition);
        xBeatPoint = qRound(xBeatPoint);
        m_beats.emplace_back(xBeatPoint, 0.0f, xBeatPoint, rendererHeight);
    }
    // Make sure to use constData to prevent detaches!
    painter->drawLines(&m_beats[0], m_beats.size());
    painter->restore();
}
