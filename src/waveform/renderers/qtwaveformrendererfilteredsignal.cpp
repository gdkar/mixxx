#include "qtwaveformrendererfilteredsignal.h"

#include "waveformwidgetrenderer.h"
#include "waveform/waveform.h"
#include "waveform/waveformwidgetfactory.h"
#include "controlobjectslave.h"
#include "trackinfoobject.h"
#include "util/math.h"

#include <QLineF>
#include <QLinearGradient>

QtWaveformRendererFilteredSignal::QtWaveformRendererFilteredSignal(
        WaveformWidgetRenderer* waveformWidgetRenderer)
    : WaveformRendererSignalBase(waveformWidgetRenderer)
{
}

QtWaveformRendererFilteredSignal::~QtWaveformRendererFilteredSignal() = default;
void QtWaveformRendererFilteredSignal::onSetup(const QDomNode& /*node*/) {
    QColor all = m_pColors->getSignalColor();
    QColor low = m_pColors->getLowColor();
    QColor mid = m_pColors->getMidColor();
    QColor high = m_pColors->getHighColor();
    all.setAlphaF(0.25);
    low.setAlphaF(0.5);
    mid.setAlphaF(0.5);
    high.setAlphaF(0.5);
    m_allBrush = QBrush(all); 
    m_lowBrush = QBrush(low);
    m_midBrush = QBrush(mid);
    m_highBrush= QBrush(high);

    low.setAlphaF(0.125);
    mid.setAlphaF(0.125);
    high.setAlphaF(0.125);

    m_lowKilledBrush = QBrush(low);
    m_midKilledBrush = QBrush(mid);
    m_highKilledBrush= QBrush(high);
}
void QtWaveformRendererFilteredSignal::onResize()
{
//    m_polygon[0].reserve(2*m_waveformRenderer->getWidth()+2);
//    m_polygon[1].reserve(2*m_waveformRenderer->getWidth()+2);
//    m_polygon[2].resize(2*m_waveformRenderer->getWidth()+2);
//    m_polygon[3].resize(2*m_waveformRenderer->getWidth()+2);
}
int QtWaveformRendererFilteredSignal::buildPolygon()
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
    m_polygon[0].clear();
    m_polygon[1].clear();
    m_polygon[2].clear();
    m_polygon[3].clear();
    m_polygon[0].reserve(2 * m_waveformRenderer->getWidth() + 2);
    m_polygon[1].reserve(2 * m_waveformRenderer->getWidth() + 2);
    m_polygon[2].reserve(2 * m_waveformRenderer->getWidth() + 2);
    m_polygon[3].reserve(2 * m_waveformRenderer->getWidth() + 2);
    QPoint point(0, 0);
    m_polygon[0].push_back(point);
    m_polygon[1].push_back(point);
    m_polygon[2].push_back(point);
    m_polygon[3].push_back(point);
    auto offset = firstVisualIndex;
    // Represents the # of waveform data points per horizontal pixel.
    auto gain = (lastVisualIndex - firstVisualIndex) / (double)m_waveformRenderer->getWidth();
    auto allGain = 1.f, lowGain = 1.f, midGain = 1.f, highGain = 1.f;
    getGains(&allGain, &lowGain, &midGain, &highGain);
    //NOTE(vrince) Please help me find a better name for "channelSeparation"
    //this variable stand for merged channel ... 1 = merged & 2 = separated
    auto channelSeparation = 2;
    if (m_alignment != Qt::AlignCenter) channelSeparation = 1;
    for (auto channel = 0; channel < channelSeparation; ++channel) {
        auto startPixel = 0;
        auto endPixel = m_waveformRenderer->getWidth() - 1;
        auto delta = 1;
        auto direction = 1.0;
        //Reverse display for merged bottom channel
        if (m_alignment == Qt::AlignBottom) direction = -1.0;
        if (channel == 1) {
            startPixel = m_waveformRenderer->getWidth() - 1;
            endPixel = 0;
            delta = -1;
            direction = -1.0;
            // After preparing the first channel, insert the pivot point.
            m_polygon[0].emplace_back(m_waveformRenderer->getWidth(), 0);
            m_polygon[1].emplace_back(m_waveformRenderer->getWidth(), 0);
            m_polygon[2].emplace_back(m_waveformRenderer->getWidth(), 0);
            m_polygon[3].emplace_back(m_waveformRenderer->getWidth(), 0);
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
            if (visualFrameStop < 0 || visualFrameStart > lastVisualFrame) {
                m_polygon[0].emplace_back(x, 0);
                m_polygon[1].emplace_back(x, 0);
                m_polygon[2].emplace_back(x, 0);
                m_polygon[3].emplace_back(x, 0);
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
            auto maxAll  = 0;
            auto maxLow  = 0;
            auto maxBand = 0;
            auto maxHigh = 0;
            for (auto i = visualIndexStart; i >= 0 && i < dataSize && i <= visualIndexStop; i += channelSeparation) {
                auto& waveformData = *(data + i);
                auto all = waveformData.filtered.all;
                auto low = waveformData.filtered.low;
                auto mid = waveformData.filtered.mid;
                auto high = waveformData.filtered.high;
                maxAll = std::max<int>(maxAll,all);
                maxLow = std::max<int>(maxLow, low);
                maxBand = std::max<int>(maxBand, mid);
                maxHigh = std::max<int>(maxHigh, high);
            }
            m_polygon[0].emplace_back(x, maxLow * lowGain * direction);
            m_polygon[1].emplace_back(x, maxBand * midGain * direction);
            m_polygon[2].emplace_back(x, maxHigh * highGain * direction);
            m_polygon[3].emplace_back(x, maxAll * direction);
        }
    }
    //If channel are not displayed separately we need to close the loop properly
    if (channelSeparation == 1)
    {
        point = QPoint(m_waveformRenderer->getWidth(), 0.0);
        m_polygon[0].push_back(point);
        m_polygon[1].push_back(point);
        m_polygon[2].push_back(point);
        m_polygon[4].push_back(point);
    }
    return m_polygon[0].size();
}
void QtWaveformRendererFilteredSignal::draw(QPainter* painter, QPaintEvent* /*event*/)
{
    auto pTrack = m_waveformRenderer->getTrackInfo();
    if (!pTrack) return;
    painter->save();
//    painter->setRenderHint(QPainter::Antialiasing);
    painter->resetTransform();
    //visual gain
    auto allGain = 1.f;
    getGains(&allGain, nullptr, nullptr, nullptr);
    auto heightGain = static_cast<float>(allGain * m_waveformRenderer->getHeight()/255.0f);
    if (m_alignment == Qt::AlignTop)
    {
        painter->translate(0.0f, 0.0f);
        painter->scale(1.0f, heightGain);
    }
    else if (m_alignment == Qt::AlignBottom)
    {
        painter->translate(0.0f, m_waveformRenderer->getHeight());
        painter->scale(1.0f, heightGain);
    } else {
        painter->translate(0.0f, m_waveformRenderer->getHeight()*0.5f);
        painter->scale(1.0f, 0.5f*heightGain);
    }
    //draw reference line
    if (m_alignment == Qt::AlignCenter) {
        painter->setPen(m_pColors->getAxesColor());
        painter->drawLine(0,0,m_waveformRenderer->getWidth(),0);
    }
    auto numberOfPoints = buildPolygon();
    if (m_pLowKillControlObject && m_pLowKillControlObject->get() > 0.1)
    {
        painter->setPen(QPen(m_lowKilledBrush, 0.0));
        painter->setBrush(QColor(150,150,150,20));
    } else {
        painter->setPen(QPen(m_lowBrush, 0.0));
        painter->setBrush(m_lowBrush);
    }
    painter->drawPolygon(&m_polygon[0][0], numberOfPoints);
    if (m_pMidKillControlObject && m_pMidKillControlObject->get() > 0.1)
    {
        painter->setPen(QPen(m_midKilledBrush, 0.0));
        painter->setBrush(QColor(150,150,150,20));
    } else {
        painter->setPen(QPen(m_midBrush, 0.0));
        painter->setBrush(m_midBrush);
    }
    painter->drawPolygon(&m_polygon[1][0], numberOfPoints);
    if (m_pHighKillControlObject && m_pHighKillControlObject->get() > 0.1)
    {
        painter->setPen(QPen(m_highKilledBrush, 0.0));
        painter->setBrush(QColor(150,150,150,20));
    } else {
        painter->setPen(QPen(m_highBrush, 0.0));
        painter->setBrush(m_highBrush);
    }
    painter->drawPolygon(&m_polygon[2][0], numberOfPoints);
    painter->setPen(QPen(m_allBrush,0.0));
    painter->setBrush(m_allBrush);
    painter->drawPolygon(&m_polygon[3][0], numberOfPoints);
    painter->restore();
}
