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
    : WaveformRendererSignalBase(waveformWidgetRenderer) {
}

QtWaveformRendererFilteredSignal::~QtWaveformRendererFilteredSignal() = default;
void QtWaveformRendererFilteredSignal::onSetup(const QDomNode& /*node*/) {
    QColor low = m_pColors->getLowColor();
    QColor mid = m_pColors->getMidColor();
    QColor high = m_pColors->getHighColor();

    QColor lowCenter = low;
    QColor midCenter = mid;
    QColor highCenter = high;

    low.setAlphaF(0.9);
    mid.setAlphaF(0.9);
    high.setAlphaF(0.9);

    lowCenter.setAlphaF(0.5);
    midCenter.setAlphaF(0.5);
    highCenter.setAlphaF(0.5);

    QLinearGradient gradientLow(QPointF(0.0,-255.0/2.0),QPointF(0.0,255.0/2.0));
    gradientLow.setColorAt(0.0, low);
    gradientLow.setColorAt(0.25,low.lighter(85));
    gradientLow.setColorAt(0.5, lowCenter.darker(115));
    gradientLow.setColorAt(0.75,low.lighter(85));
    gradientLow.setColorAt(1.0, low);
    m_lowBrush = QBrush(gradientLow);

    QLinearGradient gradientMid(QPointF(0.0,-255.0/2.0),QPointF(0.0,255.0/2.0));
    gradientMid.setColorAt(0.0, mid);
    gradientMid.setColorAt(0.35,mid.lighter(85));
    gradientMid.setColorAt(0.5, midCenter.darker(115));
    gradientMid.setColorAt(0.65,mid.lighter(85));
    gradientMid.setColorAt(1.0, mid);
    m_midBrush = QBrush(gradientMid);

    QLinearGradient gradientHigh(QPointF(0.0,-255.0/2.0),QPointF(0.0,255.0/2.0));
    gradientHigh.setColorAt(0.0, high);
    gradientHigh.setColorAt(0.45,high.lighter(85));
    gradientHigh.setColorAt(0.5, highCenter.darker(115));
    gradientHigh.setColorAt(0.55,high.lighter(85));
    gradientHigh.setColorAt(1.0, high);
    m_highBrush = QBrush(gradientHigh);

    low.setAlphaF(0.3);
    mid.setAlphaF(0.3);
    high.setAlphaF(0.3);

    QLinearGradient gradientKilledLow(QPointF(0.0,-255.0/2.0),QPointF(0.0,255.0/2.0));
    gradientKilledLow.setColorAt(0.0,low.darker(80));
    gradientKilledLow.setColorAt(0.5,lowCenter.darker(150));
    gradientKilledLow.setColorAt(1.0,low.darker(80));
    m_lowKilledBrush = QBrush(gradientKilledLow);

    QLinearGradient gradientKilledMid(QPointF(0.0,-255.0/2.0),QPointF(0.0,255.0/2.0));
    gradientKilledMid.setColorAt(0.0,mid.darker(80));
    gradientKilledMid.setColorAt(0.5,midCenter.darker(150));
    gradientKilledMid.setColorAt(1.0,mid.darker(80));
    m_midKilledBrush = QBrush(gradientKilledMid);

    QLinearGradient gradientKilledHigh(QPointF(0.0,-255.0/2.0),QPointF(0.0,255.0/2.0));
    gradientKilledHigh.setColorAt(0.0,high.darker(80));
    gradientKilledHigh.setColorAt(0.5,highCenter.darker(150));
    gradientKilledHigh.setColorAt(1.0,high.darker(80));
    m_highKilledBrush = QBrush(gradientKilledHigh);
}

void QtWaveformRendererFilteredSignal::onResize() {
    m_polygon[0].resize(2*m_waveformRenderer->getWidth()+2);
    m_polygon[1].resize(2*m_waveformRenderer->getWidth()+2);
    m_polygon[2].resize(2*m_waveformRenderer->getWidth()+2);
}
namespace{
  void setPoint(QPointF& point, qreal x, qreal y) {
      point.setX(x);
      point.setY(y);
  }
};
int QtWaveformRendererFilteredSignal::buildPolygon() {
    // We have to check the track is present because it might have been unloaded
    // between the call to draw and the call to buildPolygon
    auto pTrack = m_waveformRenderer->getTrackInfo();
    if (!pTrack) {return 0;}
    auto waveform = pTrack->getWaveform();
    if (waveform.isNull()) {return 0;}
    auto dataSize = waveform->getDataSize();
    if (dataSize <= 1) {return 0;}
    auto  data = waveform->data();
    if (!data) {return 0;}
    auto firstVisualIndex = m_waveformRenderer->getFirstDisplayedPosition() * dataSize;
    auto lastVisualIndex = m_waveformRenderer->getLastDisplayedPosition() * dataSize;
    m_polygon[0].clear();
    m_polygon[1].clear();
    m_polygon[2].clear();
    m_polygon[0].reserve(2 * m_waveformRenderer->getWidth() + 2);
    m_polygon[1].reserve(2 * m_waveformRenderer->getWidth() + 2);
    m_polygon[2].reserve(2 * m_waveformRenderer->getWidth() + 2);
    QPointF point(0.0, 0.0);
    m_polygon[0].append(point);
    m_polygon[1].append(point);
    m_polygon[2].append(point);
    auto offset = firstVisualIndex;
    // Represents the # of waveform data points per horizontal pixel.
    auto gain = (lastVisualIndex - firstVisualIndex) / (double)m_waveformRenderer->getWidth();
    float lowGain(1.0), midGain(1.0), highGain(1.0);
    getGains(NULL, &lowGain, &midGain, &highGain);
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
            point = QPointF(m_waveformRenderer->getWidth(), 0.0);
            m_polygon[0].append(point);
            m_polygon[1].append(point);
            m_polygon[2].append(point);
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
                point = QPointF(x, 0.0);
                m_polygon[0].append(point);
                m_polygon[1].append(point);
                m_polygon[2].append(point);
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

            // if (x == m_waveformRenderer->getWidth() / 2) {
            //     qDebug() << "audioVisualRatio" << waveform->getAudioVisualRatio();
            //     qDebug() << "visualSampleRate" << waveform->getVisualSampleRate();
            //     qDebug() << "audioSamplesPerVisualPixel" << waveform->getAudioSamplesPerVisualSample();
            //     qDebug() << "visualSamplePerPixel" << visualSamplePerPixel;
            //     qDebug() << "xSampleWidth" << xSampleWidth;
            //     qDebug() << "xVisualSampleIndex" << xVisualSampleIndex;
            //     qDebug() << "maxSamplingRange" << maxSamplingRange;;
            //     qDebug() << "Sampling pixel " << x << "over [" << visualIndexStart << visualIndexStop << "]";
            // }

            unsigned char maxLow = 0;
            unsigned char maxBand = 0;
            unsigned char maxHigh = 0;
            for (int i = visualIndexStart; i >= 0 && i < dataSize && i <= visualIndexStop;
                 i += channelSeparation) {
                auto& waveformData = *(data + i);
                auto low = waveformData.filtered.low;
                auto mid = waveformData.filtered.mid;
                auto high = waveformData.filtered.high;
                maxLow = math_max(maxLow, low);
                maxBand = math_max(maxBand, mid);
                maxHigh = math_max(maxHigh, high);
            }
            m_polygon[0].append(QPointF(x, (float)maxLow * lowGain * direction));
            m_polygon[1].append(QPointF(x, (float)maxBand * midGain * direction));
            m_polygon[2].append(QPointF(x, (float)maxHigh * highGain * direction));
        }
    }
    //If channel are not displayed separately we need to close the loop properly
    if (channelSeparation == 1) {
        point = QPointF(m_waveformRenderer->getWidth(), 0.0);
        m_polygon[0].append(point);
        m_polygon[1].append(point);
        m_polygon[2].append(point);
    }
    return m_polygon[0].size();
}
void QtWaveformRendererFilteredSignal::draw(QPainter* painter, QPaintEvent* /*event*/) {
    auto pTrack = m_waveformRenderer->getTrackInfo();
    if (!pTrack) return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->resetTransform();
    //visual gain
    float allGain(1.0);
    getGains(&allGain, NULL, NULL, NULL);
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
    if (m_pLowKillControlObject && m_pLowKillControlObject->get() > 0.1) {
        painter->setPen(QPen(m_lowKilledBrush, 0.0));
        painter->setBrush(QColor(150,150,150,20));
    } else {
        painter->setPen(QPen(m_lowBrush, 0.0));
        painter->setBrush(m_lowBrush);
    }
    painter->drawPolygon(&m_polygon[0][0], numberOfPoints);
    if (m_pMidKillControlObject && m_pMidKillControlObject->get() > 0.1) {
        painter->setPen(QPen(m_midKilledBrush, 0.0));
        painter->setBrush(QColor(150,150,150,20));
    } else {
        painter->setPen(QPen(m_midBrush, 0.0));
        painter->setBrush(m_midBrush);
    }
    painter->drawPolygon(&m_polygon[1][0], numberOfPoints);
    if (m_pHighKillControlObject && m_pHighKillControlObject->get() > 0.1) {
        painter->setPen(QPen(m_highKilledBrush, 0.0));
        painter->setBrush(QColor(150,150,150,20));
    } else {
        painter->setPen(QPen(m_highBrush, 0.0));
        painter->setBrush(m_highBrush);
    }
    painter->drawPolygon(&m_polygon[2][0], numberOfPoints);
    painter->restore();
}
