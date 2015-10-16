#include <QMetaEnum>
#include "dlgprefwaveform.h"

#include "mixxx.h"
#include "waveform/waveformwidgetfactory.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

DlgPrefWaveform::DlgPrefWaveform(QWidget* pParent, MixxxMainWindow* pMixxx,
                                 ConfigObject<ConfigValue>* pConfig)
        : DlgPreferencePage(pParent),
          m_pConfig(pConfig),
          m_pMixxx(pMixxx)
{
    setupUi(this);
    // Waveform overview init
    waveformOverviewComboBox->addItem(tr("Filtered")); // "0"
    waveformOverviewComboBox->addItem(tr("HSV")); // "1"
    waveformOverviewComboBox->addItem(tr("RGB")); // "2"
    // Populate waveform options.
    auto factory = WaveformWidgetFactory::instance();
    auto typeEnum = QMetaEnum::fromType<WaveformWidget::RenderType>();
    for( auto i = 0; i < typeEnum.keyCount();i++)
    {
      waveformTypeComboBox->addItem(typeEnum.key(i));
    }
    // Populate zoom options.
    for (int i = WaveformWidgetRenderer::s_waveformMinZoom;
         i <= WaveformWidgetRenderer::s_waveformMaxZoom; i++)
    {
        defaultZoomComboBox->addItem(QString::number(100/double(i), 'f', 1) + " %");
    }
    // The GUI is not fully setup so connecting signals before calling
    // slotUpdate can generate rebootMixxxView calls.
    // TODO(XXX): Improve this awkwardness.
    slotUpdate();
    connect(frameRateSpinBox, SIGNAL(valueChanged(int)),this, SLOT(slotSetFrameRate(int)));
    connect(endOfTrackWarningTimeSpinBox, SIGNAL(valueChanged(int)),this, SLOT(slotSetWaveformEndRender(int)));
    connect(frameRateSlider, SIGNAL(valueChanged(int)),frameRateSpinBox, SLOT(setValue(int)));
    connect(frameRateSpinBox, SIGNAL(valueChanged(int)),frameRateSlider, SLOT(setValue(int)));
    connect(endOfTrackWarningTimeSlider, SIGNAL(valueChanged(int)),endOfTrackWarningTimeSpinBox, SLOT(setValue(int)));
    connect(endOfTrackWarningTimeSpinBox, SIGNAL(valueChanged(int)),endOfTrackWarningTimeSlider, SLOT(setValue(int)));

    connect(waveformTypeComboBox, SIGNAL(activated(int)),this, SLOT(slotSetWaveformType(int)));
    connect(defaultZoomComboBox, SIGNAL(currentIndexChanged(int)),this, SLOT(slotSetDefaultZoom(int)));
    connect(synchronizeZoomCheckBox, SIGNAL(clicked(bool)),this, SLOT(slotSetZoomSynchronization(bool)));
    connect(allVisualGain,SIGNAL(valueChanged(double)),this,SLOT(slotSetVisualGainAll(double)));
    connect(lowVisualGain,SIGNAL(valueChanged(double)),this,SLOT(slotSetVisualGainLow(double)));
    connect(midVisualGain,SIGNAL(valueChanged(double)),this,SLOT(slotSetVisualGainMid(double)));
    connect(highVisualGain,SIGNAL(valueChanged(double)),this,SLOT(slotSetVisualGainHigh(double)));
    connect(normalizeOverviewCheckBox,SIGNAL(toggled(bool)),this,SLOT(slotSetNormalizeOverview(bool)));
    connect(waveformOverviewComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(slotSetWaveformOverviewType(int)));
}
DlgPrefWaveform::~DlgPrefWaveform() = default;
void DlgPrefWaveform::slotUpdate()
{
    auto factory = WaveformWidgetFactory::instance();
    auto currentType = factory->getType();
    auto currentIndex = waveformTypeComboBox->findData(currentType);
    if (currentIndex != -1 && waveformTypeComboBox->currentIndex() != currentIndex)
    {
        waveformTypeComboBox->setCurrentIndex(currentIndex);
    }
    frameRateSpinBox->setValue(factory->getFrameRate());
    frameRateSlider->setValue(factory->getFrameRate());
    endOfTrackWarningTimeSpinBox->setValue(factory->getEndOfTrackWarningTime());
    endOfTrackWarningTimeSlider->setValue(factory->getEndOfTrackWarningTime());
    synchronizeZoomCheckBox->setChecked(factory->isZoomSync());
    allVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::All));
    lowVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::Low));
    midVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::Mid));
    highVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::High));
    normalizeOverviewCheckBox->setChecked(factory->isOverviewNormalized());
    defaultZoomComboBox->setCurrentIndex(factory->getDefaultZoom() - 1);
    // By default we set filtered woverview = "0"
    auto overviewType = m_pConfig->getValueString(ConfigKey("Waveform","WaveformOverviewType"), "0").toInt();
    if (overviewType != waveformOverviewComboBox->currentIndex())
        waveformOverviewComboBox->setCurrentIndex(overviewType);
}
void DlgPrefWaveform::slotApply() {}
void DlgPrefWaveform::slotResetToDefaults() {
    auto factory = WaveformWidgetFactory::instance();
    // Get the default we ought to use based on whether the user has OpenGL or
    // not.
    auto defaultType = factory->autoChooseWidgetType();
    auto defaultIndex = waveformTypeComboBox->findData(defaultType);
    if (defaultIndex != -1 && waveformTypeComboBox->currentIndex() != defaultIndex)
        waveformTypeComboBox->setCurrentIndex(defaultIndex);
    allVisualGain->setValue(1.0);
    lowVisualGain->setValue(1.0);
    midVisualGain->setValue(1.0);
    highVisualGain->setValue(1.0);
    // Default zoom level is 3 in WaveformWidgetFactory.
    defaultZoomComboBox->setCurrentIndex(3 + 1);
    // Don't synchronize zoom by default.
    synchronizeZoomCheckBox->setChecked(true);
    // Filtered overview.
    waveformOverviewComboBox->setCurrentIndex(0);
    // Don't normalize overview.
    normalizeOverviewCheckBox->setChecked(false);
    // 30FPS is the default
    frameRateSlider->setValue(60);
    endOfTrackWarningTimeSlider->setValue(30);
}
void DlgPrefWaveform::slotSetFrameRate(int frameRate)
{
    WaveformWidgetFactory::instance()->setFrameRate(frameRate);
}
void DlgPrefWaveform::slotSetWaveformEndRender(int endTime)
{
    WaveformWidgetFactory::instance()->setEndOfTrackWarningTime(endTime);
}
void DlgPrefWaveform::slotSetWaveformType(int index)
{
    // Ignore sets for -1 since this happens when we clear the combobox.
    if (index < 0 || index >= QMetaEnum::fromType<WaveformWidget::RenderType>().keyCount())  return;
    auto type = QMetaEnum::fromType<WaveformWidget::RenderType>().value(index);
    WaveformWidgetFactory::instance()->setWidgetType(static_cast<WaveformWidget::RenderType>(type));
}
void DlgPrefWaveform::slotSetWaveformOverviewType(int index)
{
    m_pConfig->set(ConfigKey("Waveform","WaveformOverviewType"), ConfigValue(index));
    m_pMixxx->rebootMixxxView();
}
void DlgPrefWaveform::slotSetDefaultZoom(int index)
{
    WaveformWidgetFactory::instance()->setDefaultZoom(index + 1);
}
void DlgPrefWaveform::slotSetZoomSynchronization(bool checked)
{
    WaveformWidgetFactory::instance()->setZoomSync(checked);
}
void DlgPrefWaveform::slotSetVisualGainAll(double gain)
{
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::All,gain);
}
void DlgPrefWaveform::slotSetVisualGainLow(double gain)
{
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::Low,gain);
}
void DlgPrefWaveform::slotSetVisualGainMid(double gain)
{
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::Mid,gain);
}
void DlgPrefWaveform::slotSetVisualGainHigh(double gain)
{
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::High,gain);
}
void DlgPrefWaveform::slotSetNormalizeOverview(bool normalize)
{
    WaveformWidgetFactory::instance()->setOverviewNormalized(normalize);
}
