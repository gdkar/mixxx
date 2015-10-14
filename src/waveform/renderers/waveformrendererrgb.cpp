#include "waveformrendererrgb.h"

#include "waveformwidgetrenderer.h"
#include "waveform/waveform.h"
#include "waveform/waveformwidgetfactory.h"

#include "widget/wskincolor.h"
#include "trackinfoobject.h"
#include "widget/wwidget.h"
#include "util/math.h"

WaveformRendererRGB::WaveformRendererRGB(
        WaveformWidgetRenderer* waveformWidgetRenderer)
        : WaveformRendererSignalBase(waveformWidgetRenderer)
{
}
WaveformRendererRGB::~WaveformRendererRGB() = default;
void WaveformRendererRGB::onSetup(const QDomNode& /* node */) {}
void WaveformRendererRGB::draw(QPainter* painter,
                                          QPaintEvent* /*event*/) {
    auto trackInfo = m_waveformRenderer->getTrackInfo();
    if (!trackInfo) return;
    auto waveform = trackInfo->getWaveform();
    if (waveform.isNull()) return;
    auto dataSize = waveform->size();
    if (dataSize <= 1) return;
    auto  data = waveform->data();
    if (!data ) return;
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing, false);
    painter->setRenderHints(QPainter::HighQualityAntialiasing, false);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, false);
    painter->setWorldMatrixEnabled(false);
    painter->resetTransform();
    auto firstVisualIndex = m_waveformRenderer->getFirstDisplayedPosition() * dataSize;
    auto lastVisualIndex = m_waveformRenderer->getLastDisplayedPosition() * dataSize;
    auto offset = firstVisualIndex;

    // Represents the # of waveform data points per horizontal pixel.
    auto gain = (lastVisualIndex - firstVisualIndex) / (double)m_waveformRenderer->getWidth();
    // Per-band gain from the EQ knobs.
    float allGain(1.0), lowGain(1.0), midGain(1.0), highGain(1.0);
    getGains(&allGain, &lowGain, &midGain, &highGain);
    QColor color;
    auto halfHeight = (float)m_waveformRenderer->getHeight()/2.0f;
    auto heightFactor = allGain*halfHeight/255.0f;
    // Draw reference line
    painter->setPen(m_pColors->getAxesColor());
    painter->drawLine(0,halfHeight,m_waveformRenderer->getWidth(),halfHeight);
    for (int x = 0; x < m_waveformRenderer->getWidth(); ++x)
    {
        // Width of the x position in visual indices.
        auto xSampleWidth = gain * x;

        // Effective visual index of x
        auto xVisualSampleIndex = xSampleWidth + offset;

        // Our current pixel (x) corresponds to a number of visual samples
        // (visualSamplerPerPixel) in our waveform object. We take the max of
        // all the data points on either side of xVisualSampleIndex within a
        // window of 'maxSamplingRange' visual samples to measure the maximum
        // data point contained by this pixel.
        auto maxSamplingRange = gain * 0.5;
        // Since xVisualSampleIndex is in visual-samples (e.g. R,L,R,L) we want
        // to check +/- maxSamplingRange frames, not samples. To do this, divide
        // xVisualSampleIndex by 2. Since frames indices are integers, we round
        // to the nearest integer by adding 0.5 before casting to int.
        auto visualFrameStart = int(xVisualSampleIndex / 2.0 - maxSamplingRange + 0.5);
        auto visualFrameStop = int(xVisualSampleIndex / 2.0 + maxSamplingRange + 0.5);
        auto lastVisualFrame = dataSize / 2 - 1;
        // We now know that some subset of [visualFrameStart, visualFrameStop]
        // lies within the valid range of visual frames. Clamp
        // visualFrameStart/Stop to within [0, lastVisualFrame].
        visualFrameStart = math_clamp(visualFrameStart, 0, lastVisualFrame);
        visualFrameStop = math_clamp(visualFrameStop, 0, lastVisualFrame);
        auto visualIndexStart = visualFrameStart * 2;
        auto visualIndexStop  = visualFrameStop * 2;
        unsigned char maxLow  = 0;
        unsigned char maxMid  = 0;
        unsigned char maxHigh = 0;
        unsigned char maxAllA = 0;
        unsigned char maxAllB = 0;
        for (int i = visualIndexStart; i >= 0 && i + 1 < dataSize && i + 1 <= visualIndexStop; i += 2)
        {
            auto & waveformData = *(data + i);
            auto & waveformDataNext = *(data + i + 1);
            maxLow  = math_max3(maxLow,  waveformData.filtered.low,  waveformDataNext.filtered.low);
            maxMid  = math_max3(maxMid,  waveformData.filtered.mid,  waveformDataNext.filtered.mid);
            maxHigh = math_max3(maxHigh, waveformData.filtered.high, waveformDataNext.filtered.high);
            maxAllA = math_max(maxAllA, waveformData.filtered.all);
            maxAllB = math_max(maxAllB, waveformDataNext.filtered.all);
        }

        auto maxLowF = maxLow * lowGain;
        auto maxMidF = maxMid * midGain;
        auto maxHighF = maxHigh * highGain;

        auto red   = maxLowF * m_rgbLowColor_r + maxMidF * m_rgbMidColor_r + maxHighF * m_rgbHighColor_r;
        auto green = maxLowF * m_rgbLowColor_g + maxMidF * m_rgbMidColor_g + maxHighF * m_rgbHighColor_g;
        auto blue  = maxLowF * m_rgbLowColor_b + maxMidF * m_rgbMidColor_b + maxHighF * m_rgbHighColor_b;
        // Compute maximum (needed for value normalization)
        auto max = math_max3(red, green, blue);
        // Prevent division by zero
        if (max > 0.0f)
        {
            // Set color
            color.setRgbF(red / max, green / max, blue / max);
            painter->setPen(color);
            switch (m_alignment) {
                case Qt::AlignBottom :
                    painter->drawLine(
                        x, m_waveformRenderer->getHeight(),
                        x, m_waveformRenderer->getHeight() - (int)(heightFactor*(float)math_max(maxAllA,maxAllB)));
                    break;
                case Qt::AlignTop :
                    painter->drawLine(
                        x, 0,
                        x, (int)(heightFactor*(float)math_max(maxAllA,maxAllB)));
                    break;
                default :
                    painter->drawLine(
                        x, (int)(halfHeight-heightFactor*(float)maxAllA),
                        x, (int)(halfHeight+heightFactor*(float)maxAllB));
            }
        }
    }
    painter->restore();
}
