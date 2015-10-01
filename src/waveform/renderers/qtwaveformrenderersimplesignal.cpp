#include "qtwaveformrenderersimplesignal.h"

#include "waveformwidgetrenderer.h"
#include "waveform/waveform.h"
#include "waveform/waveformwidgetfactory.h"

#include "widget/wwidget.h"
#include "trackinfoobject.h"
#include "util/math.h"

#include <QLinearGradient>
QtWaveformRendererSimpleSignal::QtWaveformRendererSimpleSignal(WaveformWidgetRenderer* waveformWidgetRenderer) :
    WaveformRendererSignalBase(waveformWidgetRenderer) {

}
QtWaveformRendererSimpleSignal::~QtWaveformRendererSimpleSignal() = default;
void QtWaveformRendererSimpleSignal::onSetup(const QDomNode& node) {
    Q_UNUSED(node);
    QColor borderColor = m_pColors->getSignalColor().lighter(125);
    borderColor.setAlphaF(0.5);
    m_borderPen.setColor(borderColor);
    m_borderPen.setWidthF(1.25);
    QColor signalColor = m_pColors->getSignalColor();
    signalColor.setAlphaF(0.8);
    m_brush = QBrush(signalColor);
}
namespace{
  void setPoint(QPointF& point, qreal x, qreal y) {
      point.setX(x);
      point.setY(y);
  }
};
void QtWaveformRendererSimpleSignal::draw(QPainter* painter, QPaintEvent* /*event*/) {

    auto pTrack = m_waveformRenderer->getTrackInfo();
    if (!pTrack) {return;}
    auto waveform = pTrack->getWaveform();
    if (waveform.isNull()) {return;}
    auto dataSize = waveform->getDataSize();
    if (dataSize <= 1) {return;}
    auto data = waveform->data();
    if (!data) {return;}
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->resetTransform();

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
    auto firstVisualIndex = m_waveformRenderer->getFirstDisplayedPosition() * dataSize;
    auto lastVisualIndex = m_waveformRenderer->getLastDisplayedPosition() * dataSize;
    m_polygon.clear();
    m_polygon.reserve(2 * m_waveformRenderer->getWidth() + 2);
    m_polygon.append(QPointF(0.0, 0.0));
    auto offset = firstVisualIndex;
    // Represents the # of waveform data points per horizontal pixel.
    auto gain = (lastVisualIndex - firstVisualIndex) / (double)m_waveformRenderer->getWidth();

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
            m_polygon.append(QPointF(m_waveformRenderer->getWidth(), 0.0));
        }
        for (auto x = startPixel;(startPixel < endPixel) ? (x <= endPixel) : (x >= endPixel);x += delta) {
            // TODO(rryan) remove before 1.11 release. I'm seeing crashes
            // sometimes where the pointIndex is very very large. It hasn't come
            // back since adding locking, but I'm leaving this so that we can
            // get some info about it before crashing. (The crash usually
            // corrupts a lot of the stack).
            if (m_polygon.size() > 2 * m_waveformRenderer->getWidth() + 2) {
                qDebug() << "OUT OF CONTROL"
                         << 2 * m_waveformRenderer->getWidth() + 2
                         << dataSize
                         << channel << m_polygon.size() << x;
            }
            // Width of the x position in visual indices.
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
                m_polygon.append(QPointF(x, 0.0));
                continue;
            }
            // We now know that some subset of [visualFrameStart,
            // visualFrameStop] lies within the valid range of visual
            // frames. Clamp visualFrameStart/Stop to within [0,
            // lastVisualFrame].
            visualFrameStart = clamp(visualFrameStart, 0, lastVisualFrame);
            visualFrameStop = clamp(visualFrameStop, 0, lastVisualFrame);
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

            unsigned char maxAll = 0;
            for (auto i = visualIndexStart; i >= 0 && i < dataSize && i <= visualIndexStop;i += channelSeparation) {
                const auto& waveformData = *(data + i);
                auto all = waveformData.filtered.all;
                maxAll = std::max(maxAll, all);
            }
            m_polygon.append(QPointF(x, (float)maxAll * direction));
        }
    }
    //If channel are not displayed separatly we nne to close the loop properly
    if (channelSeparation == 1) {m_polygon.append(QPointF(m_waveformRenderer->getWidth(), 0.0));}
    painter->setPen(m_borderPen);
    painter->setBrush(m_brush);
    painter->drawPolygon(&m_polygon[0], m_polygon.size());
    painter->restore();
}
void QtWaveformRendererSimpleSignal::onResize() {m_polygon.resize(2*m_waveformRenderer->getWidth()+2);}
