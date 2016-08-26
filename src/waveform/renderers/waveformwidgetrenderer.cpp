#include <QPainter>

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/waveform.h"
#include "widget/wwidget.h"
#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "waveform/visualplayposition.h"
#include "util/math.h"
#include "util/performancetimer.h"

const int WaveformWidgetRenderer::s_waveformMinZoom = 1;
const int WaveformWidgetRenderer::s_waveformMaxZoom = 6;
bool WaveformWidgetRenderer::isValid() const
{
    return m_initSuccess;
}
WaveformWidgetRenderer::WaveformWidgetRenderer(const char* group, QWidget *p)
    : QWidget(p),
      m_group(group),
      m_height(-1),
      m_width(-1),

      m_firstDisplayedPosition(0.0),
      m_lastDisplayedPosition(0.0),
      m_trackPixelCount(0.0),

      m_zoomFactor(1.0),
      m_rateAdjust(0.0),
      m_visualSamplePerPixel(1.0),
      m_audioSamplePerPixel(1.0),

      // Really create some to manage those;
      m_visualPlayPosition(NULL),
      m_playPos(-1),
      m_playPosVSample(0),
      m_pRateControlObject(NULL),
      m_rate(0.0),
      m_pRateRangeControlObject(NULL),
      m_rateRange(0.0),
      m_pRateDirControlObject(NULL),
      m_rateDir(0.0),
      m_pGainControlObject(NULL),
      m_gain(1.0),
      m_pTrackSamplesControlObject(NULL),
      m_trackSamples(0.0)
{
    //qDebug() << "WaveformWidgetRenderer";
}

WaveformWidgetRenderer::~WaveformWidgetRenderer() {
    //qDebug() << "~WaveformWidgetRenderer";

    for (int i = 0; i < m_rendererStack.size(); ++i)
        delete m_rendererStack[i];

    delete m_pRateControlObject;
    delete m_pRateRangeControlObject;
    delete m_pRateDirControlObject;
    delete m_pGainControlObject;
    delete m_pTrackSamplesControlObject;

}
bool WaveformWidgetRenderer::init()
{
    //qDebug() << "WaveformWidgetRenderer::init";
    m_visualPlayPosition = VisualPlayPosition::getVisualPlayPosition(m_group);

    m_pRateControlObject = new ControlProxy(
            m_group, "rate");
    m_pRateRangeControlObject = new ControlProxy(
            m_group, "rateRange");
    m_pRateDirControlObject = new ControlProxy(
            m_group, "rate_dir");
    m_pGainControlObject = new ControlProxy(
            m_group, "total_gain");
    m_pTrackSamplesControlObject = new ControlProxy(
            m_group, "track_samples");

    for (int i = 0; i < m_rendererStack.size(); ++i) {
        if (!m_rendererStack[i]->init()) {
            return false;
        }
    }
    m_initSuccess = true;
    return true;
}
void WaveformWidgetRenderer::onPreRender()
{
    // For a valid track to render we need
    m_trackSamples = m_pTrackSamplesControlObject->get();
    if (m_trackSamples <= 0.0) {
        return;
    }

    //Fetch parameters before rendering in order the display all sub-renderers with the same values
    m_rate = m_pRateControlObject->get();
    m_rateDir = m_pRateDirControlObject->get();
    m_rateRange = m_pRateRangeControlObject->get();
    // This gain adjustment compensates for an arbitrary /2 gain chop in
    // EnginePregain. See the comment there.
    m_gain = m_pGainControlObject->get() * 2;

    //Legacy stuff (Ryan it that OK?) -> Limit our rate adjustment to < 99%, "Bad Things" might happen otherwise.
    m_rateAdjust = m_rateDir * math_min(0.99, m_rate * m_rateRange);

    //rate adjust may have change sampling per
    //vRince for the moment only more than one sample per pixel is supported
    //due to the fact we play the visual play pos modulo floor m_visualSamplePerPixel ...
    auto visualSamplePerPixel = m_zoomFactor * (1.0 + m_rateAdjust);
    m_visualSamplePerPixel = math_max(1.0, visualSamplePerPixel);

    auto pTrack = TrackPointer(m_pTrack);
    auto pWaveform = pTrack ? pTrack->getWaveform() : ConstWaveformPointer();
    if (pWaveform) {
        m_audioSamplePerPixel = m_visualSamplePerPixel * pWaveform->getAudioVisualRatio();
    } else {
        m_audioSamplePerPixel = 0.0;
    }
    auto truePlayPos = m_visualPlayPosition->getEnginePlayPos();
    // m_playPos = -1 happens, when a new track is in buffer but m_visualPlayPosition was not updated
    if (m_audioSamplePerPixel && truePlayPos != -1) {
        // Track length in pixels.
        m_trackPixelCount = static_cast<double>(m_trackSamples) / 2.0 / m_audioSamplePerPixel;

        // Ratio of half the width of the renderer to the track length in
        // pixels. Percent of the track shown in half the waveform widget.
        double displayedLengthHalf = static_cast<double>(m_width) / m_trackPixelCount / 2.0;
        // Avoid pixel jitter in play position by rounding to the nearest track
        // pixel.
        m_playPos = round(truePlayPos * m_trackPixelCount) / m_trackPixelCount; // Avoid pixel jitter in play position
        m_playPosVSample = m_playPos * m_trackPixelCount * m_visualSamplePerPixel;

        m_firstDisplayedPosition = m_playPos - displayedLengthHalf;
        m_lastDisplayedPosition = m_playPos + displayedLengthHalf;
    } else {
        m_playPos = -1; // disable renderers
    }
    update();
}
void WaveformWidgetRenderer::draw(QPainter* painter, QPaintEvent* event)
{
    //PerformanceTimer timer;
    //timer.start();
    // not ready to display need to wait until track initialization is done
    // draw only first is stack (background)
    auto stackSize = m_rendererStack.size();
    if (m_trackSamples <= 0.0 || m_playPos == -1) {
        if (stackSize) {
            m_rendererStack.front()->draw(painter, event);
        }
        return;
    } else {
        for(auto &renderer : m_rendererStack)
            renderer->draw(painter,event);

        painter->setPen(m_colors.getPlayPosColor());
        painter->drawLine(m_width/2,0,m_width/2,m_height);
        painter->setOpacity(0.5);
        painter->setPen(m_colors.getBgColor());
        painter->drawLine(m_width/2 + 1,0,m_width/2 + 1,m_height);
        painter->drawLine(m_width/2 - 1,0,m_width/2 - 1,m_height);
    }
}

void WaveformWidgetRenderer::resize(int width, int height)
{
    m_width = width;
    m_height = height;
    QWidget::resize(width,height);
    for (int i = 0; i < m_rendererStack.size(); ++i) {
        m_rendererStack[i]->setDirty(true);
        m_rendererStack[i]->onResize();
    }
}
void WaveformWidgetRenderer::setup(const QDomNode& node, const SkinContext& context)
{
    m_colors.setup(node, context);
    m_pConfig = context.getConfig();
    for(auto &renderer : m_rendererStack)
        renderer->setup(node,context);
}
void WaveformWidgetRenderer::setZoom(int zoom)
{
    //qDebug() << "WaveformWidgetRenderer::setZoom" << zoom;
    m_zoomFactor = math_clamp<double>(zoom, s_waveformMinZoom, s_waveformMaxZoom);
}
void WaveformWidgetRenderer::setTrack(TrackPointer track)
{
    m_pTrack = track;
    //used to postpone first display until track sample is actually available
    m_trackSamples = -1.0;
    for(auto &renderer : m_rendererStack)
        renderer->onSetTrack();
}
