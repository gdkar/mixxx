#include "qtwaveformrenderersimplesignal.h"

#include "waveformwidgetrenderer.h"
#include "waveform/waveform.h"
#include "waveform/waveformwidgetfactory.h"
#include "trackinfoobject.h"
#include "util/math.h"

#include <QLineF>
#include <QLinearGradient>

QtWaveformRendererSimpleSignal::QtWaveformRendererSimpleSignal(
        WaveformWidgetRenderer* waveformWidgetRenderer)
    : WaveformRendererSignalBase(waveformWidgetRenderer) {
}

QtWaveformRendererSimpleSignal::~QtWaveformRendererSimpleSignal() = default;
void QtWaveformRendererSimpleSignal::onSetup(const QDomNode& /*node*/)
{
    m_allBrush = QBrush(m_pColors->getSignalColor());
}
void QtWaveformRendererSimpleSignal::onResize()
{
    m_polygon.resize(2*m_waveformRenderer->getWidth()+2);
}
int QtWaveformRendererSimpleSignal::buildPolygon()
{
    // We have to check the track is present because it might have been unloaded
    // between the call to draw and the call to buildPolygon
    auto pTrack = m_waveformRenderer->getTrackInfo();
    if (!pTrack) {return 0;}
    auto waveform = pTrack->getWaveform();
    if (waveform.isNull()) {return 0;}
    auto dataSize = waveform->size();
    if (dataSize <= 1) {return 0;}
    auto  data = waveform->data();
    if (!data) {return 0;}
    auto firstVisualIndex = m_waveformRenderer->getFirstDisplayedPosition() * dataSize;
    auto lastVisualIndex = m_waveformRenderer->getLastDisplayedPosition() * dataSize;
    m_polygon.clear();
    m_polygon.reserve(2 * m_waveformRenderer->getWidth() + 2);
    QPoint point(0, 0);
    m_polygon.push_back(point);
    auto offset = firstVisualIndex;
    // Represents the # of waveform data points per horizontal pixel.
    auto gain = (lastVisualIndex - firstVisualIndex) / (double)m_waveformRenderer->getWidth();
    //NOTE(vrince) Please help me find a better name for "channelSeparation"
    //this variable stand for merged channel ... 1 = merged & 2 = separated
    auto channelSeparation = 2;
    if (m_alignment != Qt::AlignCenter) channelSeparation = 1;
    for (auto channel = 0; channel < channelSeparation; ++channel)
    {
        auto startPixel = 0;
        auto endPixel = m_waveformRenderer->getWidth() - 1;
        auto delta = 1;
        auto direction = 1.0;
        //Reverse display for merged bottom channel
        if (m_alignment == Qt::AlignBottom) direction = -1.0;
        if (channel == 1)
        {
            startPixel = m_waveformRenderer->getWidth() - 1;
            endPixel = 0;
            delta = -1;
            direction = -1.0;
            // After preparing the first channel, insert the pivot point.
            point = QPoint(m_waveformRenderer->getWidth(), 0.0);
            m_polygon.push_back(point);
        }
        for (auto x = startPixel; (startPixel < endPixel) ? (x <= endPixel) : (x >= endPixel); x += delta) {
            auto xSampleWidth = gain * x;
            // Effective visual index of x
            auto xVisualSampleIndex = xSampleWidth + offset;
            // Our current pixel (x) corresponds to a number of visual samples
            // (visualSamplerPerPixel) in our waveform object. We take the max of
            // all the data points on either side of xVisualSampleIndex within a
            // window of 'maxSamplingRange' visual samples to measure the maximum
            // data point contained by this pixel.
            auto maxSamplingRange = gain / 2.0;
            // Since xVisualSampleIndex is in visual-samples (e.g. R,L,R,L) we want
            // to check +/- maxSamplingRange frames, not samples. To do this, divide
            // xVisualSampleIndex by 2. Since frames indices are integers, we round
            // to the nearest integer by adding 0.5 before casting to int.
            auto visualFrameStart = int(xVisualSampleIndex / 2.0 - maxSamplingRange + 0.5);
            auto visualFrameStop = int(xVisualSampleIndex / 2.0 + maxSamplingRange + 0.5);
            // If the entire sample range is off the screen then don't calculate a
            // point for this pixel.
            auto lastVisualFrame = dataSize / 2 - 1;
            if (visualFrameStop < 0 || visualFrameStart > lastVisualFrame)
            {
                m_polygon.emplace_back(x, 0);
                continue;
            }
            // We now know that some subset of [visualFrameStart,
            // visualFrameStop] lies within the valid range of visual
            // frames. Clamp visualFrameStart/Stop to within [0,
            // lastVisualFrame].
            visualFrameStart = math_clamp(visualFrameStart, 0, lastVisualFrame);
            visualFrameStop = math_clamp(visualFrameStop, 0, lastVisualFrame);
            auto visualIndexStart = visualFrameStart * 2 + channel;
            auto visualIndexStop = visualFrameStop * 2 + channel;
            auto maxAll = 0;
            for (auto i = visualIndexStart; i >= 0 && i < dataSize && i <= visualIndexStop; i += channelSeparation)
            {
                maxAll= std::max<int>(maxAll, data[i].filtered.all);
            }
            m_polygon.emplace_back(x, maxAll * direction);
        }
    }
    //If channel are not displayed separately we need to close the loop properly
    if (channelSeparation == 1)
    {
        point = QPoint(m_waveformRenderer->getWidth(), 0);
        m_polygon.push_back(point);
    }
    return m_polygon.size();
}
void QtWaveformRendererSimpleSignal::draw(QPainter* painter, QPaintEvent* /*event*/)
{
    auto pTrack = m_waveformRenderer->getTrackInfo();
    if (!pTrack) return;
    painter->save();
//    painter->setRenderHint(QPainter::Antialiasing);
    painter->resetTransform();
    //visual gain
    float allGain(1.0);
    getGains(&allGain, nullptr, nullptr, nullptr);
    auto heightGain = allGain * (double)m_waveformRenderer->getHeight()/255.0;
    if (m_alignment == Qt::AlignTop) {
        painter->translate(0.0, 0.0);
        painter->scale(1.0, heightGain);
    } else if (m_alignment == Qt::AlignBottom) {
        painter->translate(0.0, m_waveformRenderer->getHeight());
        painter->scale(1.0, heightGain);
    } else {
        painter->translate(0.0, m_waveformRenderer->getHeight()/2.0);
        painter->scale(1.0, 0.5*heightGain);
    }
    //draw reference line
    if (m_alignment == Qt::AlignCenter) {
        painter->setPen(m_pColors->getAxesColor());
        painter->drawLine(0,0,m_waveformRenderer->getWidth(),0);
    }
    auto numberOfPoints = buildPolygon();
    painter->setPen(QPen(m_allBrush, 0.0));
    painter->setBrush(m_allBrush);
    painter->drawPolygon(&m_polygon[0], numberOfPoints);
    painter->restore();
}
