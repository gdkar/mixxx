#include <QBrush>
#include <QPen>
#include <QPainter>
#include <QPolygonF>

#include "waveform/renderers/waveformrendererpreroll.h"

#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/waveform.h"
#include "widget/wskincolor.h"
#include "widget/wwidget.h"

WaveformRendererPreroll::WaveformRendererPreroll(WaveformWidgetRenderer* waveformWidgetRenderer)
  : WaveformRendererAbstract(waveformWidgetRenderer) {
}

WaveformRendererPreroll::~WaveformRendererPreroll() = default;
void WaveformRendererPreroll::setup(const QDomNode& node, const SkinContext& context)
{
    m_color.setNamedColor(context.selectString(node, "SignalColor"));
    m_color = WSkinColor::getCorrectColor(m_color);
}

void WaveformRendererPreroll::draw(QPainter* painter, QPaintEvent* event)
{
    Q_UNUSED(event);
    auto track = m_waveformRenderer->getTrackInfo();
    if (!track)  return;
    auto samplesPerPixel = m_waveformRenderer->getVisualSamplePerPixel();
    auto numberOfSamples = m_waveformRenderer->getWidth() * samplesPerPixel;
    auto currentPosition = m_waveformRenderer->getPlayPosVSample();
    //qDebug() << "currentPosition" << currentPosition
    //         << "samplesPerPixel" << samplesPerPixel
    //         << "numberOfSamples" << numberOfSamples;

    // Some of the pre-roll is on screen. Draw little triangles to indicate
    // where the pre-roll is located.
    if (currentPosition < numberOfSamples / 2.0)
    {
        auto index = static_cast<int>(numberOfSamples / 2.0 - currentPosition);
        auto polyWidth = static_cast<int>(40.0 / samplesPerPixel);
        auto halfHeight = m_waveformRenderer->getHeight()/2.0;
        auto halfPolyHeight = m_waveformRenderer->getHeight()/5.0;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        //painter->setRenderHint(QPainter::HighQualityAntialiasing);
        //painter->setBackgroundMode(Qt::TransparentMode);
        painter->setWorldMatrixEnabled(false);
        painter->setPen(QPen(QBrush(m_color), 1));
        QPolygonF polygon;
        polygon << QPointF(0, halfHeight)
                << QPointF(-polyWidth, halfHeight - halfPolyHeight)
                << QPointF(-polyWidth, halfHeight + halfPolyHeight);
        // Draw at most one not or halve visible polygon at the widget borders
        if (index > (numberOfSamples + ((polyWidth + 1) * samplesPerPixel)))
        {
            auto rest = static_cast<int>(index - numberOfSamples);
            rest %= static_cast<int>((polyWidth + 1) * samplesPerPixel);
            index = numberOfSamples + rest;
        }
        polygon.translate(((qreal)index) / samplesPerPixel, 0);
        while (index > 0)
        {
            painter->drawPolygon(polygon);
            polygon.translate(-(polyWidth + 1), 0);
            index -= (polyWidth + 1) * samplesPerPixel;
        }
        painter->restore();
    }
}
