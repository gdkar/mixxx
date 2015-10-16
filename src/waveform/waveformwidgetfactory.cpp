#include <QStringList>
#include <QMetaEnum>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QtDebug>

#include "waveform/waveformwidgetfactory.h"
#include "controlpotmeter.h"
#include "waveform/widgets/waveformwidget.h"
#include "waveform/guitick.h"
#include "widget/wwaveformviewer.h"
#include "util/cmdlineargs.h"
#include "util/performancetimer.h"
#include "util/timer.h"
#include "util/math.h"
#include "mixxx.h"
///////////////////////////////////////////

///////////////////////////////////////////
WaveformWidgetHolder::WaveformWidgetHolder()
    : m_waveformWidget(nullptr),
      m_waveformViewer(nullptr),
      m_skinContextCache(nullptr, QString())
{
}
WaveformWidgetHolder::WaveformWidgetHolder(WaveformWidget* waveformWidget,WWaveformViewer* waveformViewer,const QDomNode& node,const SkinContext& skinContext)
    : m_waveformWidget(waveformWidget),
      m_waveformViewer(waveformViewer),
      m_skinNodeCache(node.cloneNode()),
      m_skinContextCache(skinContext)
{
}
///////////////////////////////////////////
WaveformWidgetFactory::WaveformWidgetFactory() :
        m_type(WaveformWidget::Empty),
        m_config(0),
        m_skipRender(false),
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
WaveformWidgetFactory::~WaveformWidgetFactory()
{
    destroyWidgets();
}
bool WaveformWidgetFactory::setConfig(ConfigObject<ConfigValue> *config) {
    m_config = config;
    if (!m_config) return false;
    auto ok = false;
    auto frameRate = m_config->getValueString(ConfigKey("Waveform","FrameRate")).toInt(&ok);
    if (ok) setFrameRate(frameRate);
    else    m_config->set(ConfigKey("Waveform","FrameRate"), ConfigValue(m_frameRate));
    auto endTime = m_config->getValueString(ConfigKey("Waveform","EndOfTrackWarningTime")).toInt(&ok);
    if (ok) setEndOfTrackWarningTime(endTime);
    else    m_config->set(ConfigKey("Waveform","EndOfTrackWarningTime"), ConfigValue(m_endOfTrackWarningTime));
    auto vsync = m_config->getValueString(ConfigKey("Waveform","VSync"),"0").toInt();
    auto defaultZoom = m_config->getValueString(ConfigKey("Waveform","DefaultZoom")).toInt(&ok);
    if (ok) setDefaultZoom(defaultZoom);
    else m_config->set(ConfigKey("Waveform","DefaultZoom"), ConfigValue(m_defaultZoom));
    auto zoomSync = m_config->getValueString(ConfigKey("Waveform","ZoomSynchronization")).toInt(&ok);
    if (ok) {
        setZoomSync(static_cast<bool>(zoomSync));
    } else  m_config->set(ConfigKey("Waveform","ZoomSynchronization"), ConfigValue(m_zoomSync));
    auto typeString = m_config->getValueString(ConfigKey("Waveform","WaveformType"));
    auto type = static_cast<WaveformWidget::RenderType>(QMetaEnum::fromType<WaveformWidget::RenderType>().keyToValue(qPrintable(typeString),&ok));
    if (!ok || !setWidgetType(type)) 
      setWidgetType(autoChooseWidgetType());
    for (auto i = 0; i < FilterCount; i++) {
        auto visualGain = m_config->getValueString(ConfigKey("Waveform","VisualGain_" + QString::number(i))).toDouble(&ok);
        if (ok) setVisualGain(FilterIndex(i), visualGain);
        else  m_config->set(ConfigKey("Waveform","VisualGain_" + QString::number(i)),QString::number(m_visualGain[i]));
    }
    int overviewNormalized = m_config->getValueString(ConfigKey("Waveform","OverviewNormalized")).toInt(&ok);
    if (ok) {
        setOverviewNormalized(static_cast<bool>(overviewNormalized));
    } else m_config->set(ConfigKey("Waveform","OverviewNormalized"), ConfigValue(m_overviewNormalized));
    return true;
}

void WaveformWidgetFactory::destroyWidgets()
{
    for ( auto & holder : m_waveformWidgetHolders)
    {
      if ( auto ptr = std::exchange(holder.m_waveformWidget,nullptr)) delete ptr;
    }
    m_waveformWidgetHolders.clear();
}
void WaveformWidgetFactory::addTimerListener(QWidget* pWidget) {
    // Do not hold the pointer to of timer listeners since they may be deleted.
    // We don't activate update() or repaint() directly so listener widgets
    // can decide whether to paint or not.
    connect(this, SIGNAL(waveformUpdateTick()),pWidget, SLOT(maybeUpdate()),Qt::DirectConnection);
}
bool WaveformWidgetFactory::setWaveformWidget(WWaveformViewer* viewer,const QDomElement& node,const SkinContext& context)
{
    auto index = findIndexOf(viewer);
    if (index != -1) {
        qDebug() << "WaveformWidgetFactory::setWaveformWidget - "\
                    "viewer already have a waveform widget but it's not found by the factory !";
        delete viewer->getWaveformWidget();
    }
    // Cast to widget done just after creation because it can't be perform in
    // constructor (pure virtual)
    auto waveformWidget = createWaveformWidget(m_type, viewer);
    viewer->setWaveformWidget(waveformWidget);
    viewer->setup(node, context);
    // create new holder
    if (index == -1)
    {
        m_waveformWidgetHolders.push_back( WaveformWidgetHolder(waveformWidget, viewer, node, context));
        index = m_waveformWidgetHolders.size() - 1;
    } 
    else m_waveformWidgetHolders[index] = WaveformWidgetHolder(waveformWidget, viewer, node, context);
    viewer->setZoom(m_defaultZoom);
    viewer->update();
    qDebug() << "WaveformWidgetFactory::setWaveformWidget - waveform widget added in factory, index" << index;
    return true;
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
bool WaveformWidgetFactory::setWidgetType(WaveformWidget::RenderType type) {
    if (type == m_type) return true;
    // check if type is acceptable
    m_type = type;
    auto typeString = QString(QMetaEnum::fromType<WaveformWidget::RenderType>().valueToKey(m_type));
    if (m_config) m_config->set(ConfigKey("Waveform","WaveformType"), ConfigValue(typeString));
    qDebug() << "Settint Waveform type to " << m_type;
    for ( auto & holder : m_waveformWidgetHolders)
    {
      auto viewer = holder.m_waveformViewer;
      auto widget = holder.m_waveformWidget;
      auto pTrack = widget->getTrackInfo();
      widget->setRenderType(m_type);
      viewer->setup(holder.m_skinNodeCache,holder.m_skinContextCache);
      widget->resize(viewer->width(),viewer->height());
      widget->setTrack(pTrack);
      widget->show();
      viewer->update();
    }
    m_skipRender = false;
    return true;
}

void WaveformWidgetFactory::setDefaultZoom(int zoom) {
    m_defaultZoom = math_clamp(zoom, WaveformWidgetRenderer::s_waveformMinZoom, WaveformWidgetRenderer::s_waveformMaxZoom);
    if (m_config)  m_config->set(ConfigKey("Waveform","DefaultZoom"), ConfigValue(m_defaultZoom));
    for (int i = 0; i < m_waveformWidgetHolders.size(); i++)  m_waveformWidgetHolders[i].m_waveformViewer->setZoom(m_defaultZoom);
}

void WaveformWidgetFactory::setZoomSync(bool sync) {
    m_zoomSync = sync;
    if (m_config)  m_config->set(ConfigKey("Waveform","ZoomSynchronization"), ConfigValue(m_zoomSync));
    if (m_waveformWidgetHolders.size() == 0)  return;
    int refZoom = m_waveformWidgetHolders[0].m_waveformWidget->getZoomFactor();
    for (int i = 1; i < m_waveformWidgetHolders.size(); i++)  m_waveformWidgetHolders[i].m_waveformViewer->setZoom(refZoom);
}
void WaveformWidgetFactory::setVisualGain(FilterIndex index, double gain) {
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
    if (pWaveformWidget != NULL && isZoomSync()) {
        //qDebug() << "WaveformWidgetFactory::notifyZoomChange";
        int refZoom = pWaveformWidget->getZoomFactor();
        for (int i = 0; i < m_waveformWidgetHolders.size(); ++i)
        {
            auto & holder = m_waveformWidgetHolders[i];
            if (holder.m_waveformViewer != viewer)  holder.m_waveformViewer->setZoom(refZoom);
        }
    }
}
void WaveformWidgetFactory::render()
{
    ScopedTimer t("WaveformWidgetFactory::render() %1waveforms", m_waveformWidgetHolders.size());
    //int paintersSetupTime0 = 0;
    //int paintersSetupTime1 = 0;
    if (!m_skipRender)
    {
        if (m_type)
        {   // no regular updates for an empty waveform
            // next rendered frame is displayed after next buffer swap and than after VSync
            for (int i = 0; i < m_waveformWidgetHolders.size(); i++)
                // Calculate play position for the new Frame in following run
                m_waveformWidgetHolders[i].m_waveformWidget->onPreRender(0);
            // It may happen that there is an artificially delayed due to
            // anti tearing driver settings
            // all render commands are delayed until the swap from the previous run is executed
            for (int i = 0; i < m_waveformWidgetHolders.size(); i++)
            {
                auto pWaveformWidget = m_waveformWidgetHolders[i].m_waveformWidget;
                if (pWaveformWidget->getWidth() > 0 && pWaveformWidget->isVisible()) (void)pWaveformWidget->render();
            }
        }
        // Notify all other waveform-like widgets (e.g. WSpinny's) that they should
        // update.
        emit(waveformUpdateTick());
    }
}
WaveformWidget::RenderType WaveformWidgetFactory::autoChooseWidgetType() const
{
  return WaveformWidget::Filtered;
}
WaveformWidget* WaveformWidgetFactory::createWaveformWidget(WaveformWidget::RenderType type, WWaveformViewer* viewer) 
{
    auto widget = new WaveformWidget(viewer->getGroup(), viewer);
    widget->setRenderType(type);
    return widget;
}
int WaveformWidgetFactory::findIndexOf(WWaveformViewer* viewer) const
{
    for (int i = 0; i < (int)m_waveformWidgetHolders.size(); i++)
    {
        if (m_waveformWidgetHolders[i].m_waveformViewer == viewer) return i;
    }
    return -1;
}
void WaveformWidgetFactory::startVSync(MixxxMainWindow* mixxxMainWindow)
{
    m_updateTimer.setTimerType(Qt::PreciseTimer);
    m_updateTimer.setInterval(1e3/m_frameRate);
    connect(&m_updateTimer,&QTimer::timeout,this,&WaveformWidgetFactory::render);
    auto guiTick = mixxxMainWindow->getGuiTick();
    connect(&m_updateTimer,&QTimer::timeout,guiTick,&GuiTick::process);
    m_updateTimer.start();
}
