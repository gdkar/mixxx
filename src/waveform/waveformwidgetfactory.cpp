#include <QStringList>
#include <QMetaEnum>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QtDebug>

#include "waveform/waveformwidgetfactory.h"
#include "controlpotmeter.h"
#include "waveform/widgets/waveformwidget.h"
#include "widget/wwaveformviewer.h"
#include "util/cmdlineargs.h"
#include "util/performancetimer.h"
#include "util/timer.h"
#include "util/math.h"
///////////////////////////////////////////

///////////////////////////////////////////
///////////////////////////////////////////
WaveformWidgetFactory::WaveformWidgetFactory() :
        m_type(WaveformWidget::Empty),
        m_config(0),
        m_frameRate(30),
        m_endOfTrackWarningTime(60),
        m_defaultZoom(3),
        m_zoomSync(false),
        m_overviewNormalized(false),
        m_frameCnt(0),
        m_actualFrameRate(0)
{
    m_visualGain[All] = 1.0;
    m_visualGain[Low] = 1.0;
    m_visualGain[Mid] = 1.0;
    m_visualGain[High] = 1.0;

    m_time.start();
}
WaveformWidgetFactory::~WaveformWidgetFactory() = default;
bool WaveformWidgetFactory::setConfig(ConfigObject<ConfigValue> *config)
{
    m_config = config;
    if (!m_config) return false;
    auto ok = false;
    auto frameRate = m_config->getValueString(ConfigKey("Waveform","FrameRate")).toInt(&ok);
    if (ok) setFrameRate(frameRate);
    else    m_config->set(ConfigKey("Waveform","FrameRate"), ConfigValue(m_frameRate));
    auto endTime = m_config->getValueString(ConfigKey("Waveform","EndOfTrackWarningTime")).toInt(&ok);
    if (ok) setEndOfTrackWarningTime(endTime);
    else    m_config->set(ConfigKey("Waveform","EndOfTrackWarningTime"), ConfigValue(m_endOfTrackWarningTime));
    auto defaultZoom = m_config->getValueString(ConfigKey("Waveform","DefaultZoom")).toInt(&ok);
    if (ok) setDefaultZoom(defaultZoom);
    else m_config->set(ConfigKey("Waveform","DefaultZoom"), ConfigValue(m_defaultZoom));
    auto zoomSync = m_config->getValueString(ConfigKey("Waveform","ZoomSynchronization")).toInt(&ok);
    if (ok)setZoomSync(static_cast<bool>(zoomSync));
    else  m_config->set(ConfigKey("Waveform","ZoomSynchronization"), ConfigValue(m_zoomSync));
    auto typeString = m_config->getValueString(ConfigKey("Waveform","WaveformType"));
    auto type = static_cast<WaveformWidget::RenderType>(QMetaEnum::fromType<WaveformWidget::RenderType>().keyToValue(qPrintable(typeString),&ok));
    if (!ok || !setWidgetType(type)) setWidgetType(autoChooseWidgetType());
    for (auto i = 0; i < FilterCount; i++)
    {
        auto visualGain = m_config->getValueString(ConfigKey("Waveform","VisualGain_" + QString::number(i))).toDouble(&ok);
        if (ok) setVisualGain(FilterIndex(i), visualGain);
        else  m_config->set(ConfigKey("Waveform","VisualGain_" + QString::number(i)),QString::number(m_visualGain[i]));
    }
    int overviewNormalized = m_config->getValueString(ConfigKey("Waveform","OverviewNormalized")).toInt(&ok);
    if (ok) setOverviewNormalized(static_cast<bool>(overviewNormalized));
    else m_config->set(ConfigKey("Waveform","OverviewNormalized"), ConfigValue(m_overviewNormalized));
    return true;
}
void WaveformWidgetFactory::addTimerListener(QWidget* pWidget)
{
    // Do not hold the pointer to of timer listeners since they may be deleted.
    // We don't activate update() or repaint() directly so listener widgets
    // can decide whether to paint or not.
    connect(this, SIGNAL(waveformUpdateTick()),pWidget, SLOT(maybeUpdate()),Qt::DirectConnection);
}
void WaveformWidgetFactory::setFrameRate(int frameRate)
{
    m_frameRate = math_clamp(frameRate, 1, 120);
    if (m_config) m_config->set(ConfigKey("Waveform","FrameRate"), ConfigValue(m_frameRate));
    m_updateTimer.setInterval(1e3/m_frameRate);
}
void WaveformWidgetFactory::setEndOfTrackWarningTime(int endTime)
{
    m_endOfTrackWarningTime = endTime;
    if (m_config) m_config->set(ConfigKey("Waveform","EndOfTrackWarningTime"), ConfigValue(m_endOfTrackWarningTime));
}
bool WaveformWidgetFactory::setWidgetType(WaveformWidget::RenderType type)
{
    // check if type is acceptable
    m_type = type;
    auto typeString = QString(QMetaEnum::fromType<WaveformWidget::RenderType>().valueToKey(m_type));
    if (m_config) m_config->set(ConfigKey("Waveform","WaveformType"), ConfigValue(typeString));
    qDebug() << "Settint Waveform type to " << m_type;
    emit renderTypeChanged(m_type);
    return true;
}
void WaveformWidgetFactory::setDefaultZoom(int zoom)
{
    m_defaultZoom = math_clamp(zoom, WaveformWidgetRenderer::s_waveformMinZoom, WaveformWidgetRenderer::s_waveformMaxZoom);
    if (m_config)  m_config->set(ConfigKey("Waveform","DefaultZoom"), ConfigValue(m_defaultZoom));
    emit zoomChanged(m_defaultZoom);
}
void WaveformWidgetFactory::setZoomSync(bool sync)
{
    m_zoomSync = sync;
    if (m_config)  m_config->set(ConfigKey("Waveform","ZoomSynchronization"), ConfigValue(m_zoomSync));
}
void WaveformWidgetFactory::setVisualGain(FilterIndex index, double gain)
{
    m_visualGain[index] = gain;
    if (m_config) m_config->set(ConfigKey("Waveform","VisualGain_" + QString::number(index)), QString::number(m_visualGain[index]));
}
double WaveformWidgetFactory::getVisualGain(FilterIndex index) const
{
    return m_visualGain[index];
}
void WaveformWidgetFactory::setOverviewNormalized(bool normalize)
{
    m_overviewNormalized = normalize;
    if (m_config) m_config->set(ConfigKey("Waveform","OverviewNormalized"), ConfigValue(m_overviewNormalized));
}
void WaveformWidgetFactory::notifyZoomChange(WWaveformViewer* viewer)
{
    auto pWaveformWidget = viewer->getWaveformWidget();
    if (pWaveformWidget  && isZoomSync())
    {
        //qDebug() << "WaveformWidgetFactory::notifyZoomChange";
        auto refZoom = pWaveformWidget->getZoomFactor();
        emit zoomChanged(refZoom);
    }
}
void WaveformWidgetFactory::onRender()
{
    emit preRender(0);
    emit render();
    emit(waveformUpdateTick());
}
WaveformWidget::RenderType WaveformWidgetFactory::autoChooseWidgetType() const
{
  return WaveformWidget::Filtered;
}
void WaveformWidgetFactory::startVSync()
{
    m_updateTimer.setTimerType(Qt::PreciseTimer);
    m_updateTimer.setInterval(1e3/m_frameRate);
    connect(&m_updateTimer,&QTimer::timeout,this,&WaveformWidgetFactory::onRender);
    m_updateTimer.start();
}
