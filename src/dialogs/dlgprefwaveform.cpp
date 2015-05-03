#include "dialogs/dlgprefwaveform.h"

#include "mixxx.h"
#include "waveform/waveformwidgetfactory.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

DlgPrefWaveform::DlgPrefWaveform(QWidget* pParent, MixxxMainWindow* pMixxx,
                                 ConfigObject<ConfigValue>* pConfig)
        : DlgPreferencePage(pParent),
          m_pConfig(pConfig),
          m_pMixxx(pMixxx) {
    setupUi(this);

    // Waveform overview init
    waveformOverviewComboBox->addItem(tr("Filtered")); // "0"
    waveformOverviewComboBox->addItem(tr("HSV")); // "1"
    waveformOverviewComboBox->addItem(tr("RGB")); // "2"

    // Populate waveform options.
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();
    QVector<WaveformWidgetAbstractHandle> handles = factory->getAvailableTypes();
    for (int i = 0; i < handles.size(); ++i) {
        waveformTypeComboBox->addItem(handles[i].getDisplayName(),
                                      handles[i].getType());
    }

    // Populate zoom options.
    for (int i = WaveformWidgetRenderer::s_waveformMinZoom;
         i <= WaveformWidgetRenderer::s_waveformMaxZoom; i++) {
        defaultZoomComboBox->addItem(QString::number(100/double(i), 'f', 1) + " %");
    }

    // The GUI is not fully setup so connecting signals before calling
    // onUpdate can generate rebootMixxxView calls.
    // TODO(XXX): Improve this awkwardness.
    onUpdate();
    connect(frameRateSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(onSetFrameRate(int)));
    connect(endOfTrackWarningTimeSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(onSetWaveformEndRender(int)));
    connect(frameRateSlider, SIGNAL(valueChanged(int)),
            frameRateSpinBox, SLOT(setValue(int)));
    connect(frameRateSpinBox, SIGNAL(valueChanged(int)),
            frameRateSlider, SLOT(setValue(int)));
    connect(endOfTrackWarningTimeSlider, SIGNAL(valueChanged(int)),
            endOfTrackWarningTimeSpinBox, SLOT(setValue(int)));
    connect(endOfTrackWarningTimeSpinBox, SIGNAL(valueChanged(int)),
            endOfTrackWarningTimeSlider, SLOT(setValue(int)));

    connect(waveformTypeComboBox, SIGNAL(activated(int)),
            this, SLOT(onSetWaveformType(int)));
    connect(defaultZoomComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onSetDefaultZoom(int)));
    connect(synchronizeZoomCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(onSetZoomSynchronization(bool)));
    connect(allVisualGain,SIGNAL(valueChanged(double)),
            this,SLOT(onSetVisualGainAll(double)));
    connect(lowVisualGain,SIGNAL(valueChanged(double)),
            this,SLOT(onSetVisualGainLow(double)));
    connect(midVisualGain,SIGNAL(valueChanged(double)),
            this,SLOT(onSetVisualGainMid(double)));
    connect(highVisualGain,SIGNAL(valueChanged(double)),
            this,SLOT(onSetVisualGainHigh(double)));
    connect(normalizeOverviewCheckBox,SIGNAL(toggled(bool)),
            this,SLOT(onSetNormalizeOverview(bool)));
    connect(factory, SIGNAL(waveformMeasured(float,int)),
            this, SLOT(onWaveformMeasured(float,int)));
    connect(waveformOverviewComboBox,SIGNAL(currentIndexChanged(int)),
            this,SLOT(onSetWaveformOverviewType(int)));
}

DlgPrefWaveform::~DlgPrefWaveform() {
}

void DlgPrefWaveform::onUpdate() {
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();

    if (factory->isOpenGLAvailable()) {
        openGlStatusIcon->setText(factory->getOpenGLVersion());
    } else {
        openGlStatusIcon->setText(tr("OpenGL not available"));
    }

    WaveformWidgetType::Type currentType = factory->getType();
    int currentIndex = waveformTypeComboBox->findData(currentType);
    if (currentIndex != -1 && waveformTypeComboBox->currentIndex() != currentIndex) {
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
    int overviewType = m_pConfig->getValueString(
            ConfigKey("[Waveform]","WaveformOverviewType"), "0").toInt();
    if (overviewType != waveformOverviewComboBox->currentIndex()) {
        waveformOverviewComboBox->setCurrentIndex(overviewType);
    }
}

void DlgPrefWaveform::onApply() {
}

void DlgPrefWaveform::onResetToDefaults() {
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();

    // Get the default we ought to use based on whether the user has OpenGL or
    // not.
    WaveformWidgetType::Type defaultType = factory->autoChooseWidgetType();
    int defaultIndex = waveformTypeComboBox->findData(defaultType);
    if (defaultIndex != -1 && waveformTypeComboBox->currentIndex() != defaultIndex) {
        waveformTypeComboBox->setCurrentIndex(defaultIndex);
    }

    allVisualGain->setValue(1.0);
    lowVisualGain->setValue(1.0);
    midVisualGain->setValue(1.0);
    highVisualGain->setValue(1.0);

    // Default zoom level is 3 in WaveformWidgetFactory.
    defaultZoomComboBox->setCurrentIndex(3 + 1);

    // Don't synchronize zoom by default.
    synchronizeZoomCheckBox->setChecked(false);

    // Filtered overview.
    waveformOverviewComboBox->setCurrentIndex(0);

    // Don't normalize overview.
    normalizeOverviewCheckBox->setChecked(false);

    // 30FPS is the default
    frameRateSlider->setValue(30);
    endOfTrackWarningTimeSlider->setValue(30);
}

void DlgPrefWaveform::onSetFrameRate(int frameRate) {
    WaveformWidgetFactory::instance()->setFrameRate(frameRate);
}
void DlgPrefWaveform::onSetWaveformEndRender(int endTime) {
    WaveformWidgetFactory::instance()->setEndOfTrackWarningTime(endTime);
}
void DlgPrefWaveform::onSetWaveformType(int index) {
    // Ignore sets for -1 since this happens when we clear the combobox.
    if (index < 0) {
        return;
    }
    WaveformWidgetFactory::instance()->setWidgetTypeFromHandle(index);
}

void DlgPrefWaveform::onSetWaveformOverviewType(int index) {
    m_pConfig->set(ConfigKey("[Waveform]","WaveformOverviewType"), ConfigValue(index));
    m_pMixxx->rebootMixxxView();
}

void DlgPrefWaveform::onSetDefaultZoom(int index) {
    WaveformWidgetFactory::instance()->setDefaultZoom(index + 1);
}

void DlgPrefWaveform::onSetZoomSynchronization(bool checked) {
    WaveformWidgetFactory::instance()->setZoomSync(checked);
}

void DlgPrefWaveform::onSetVisualGainAll(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::All,gain);
}

void DlgPrefWaveform::onSetVisualGainLow(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::Low,gain);
}

void DlgPrefWaveform::onSetVisualGainMid(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::Mid,gain);
}

void DlgPrefWaveform::onSetVisualGainHigh(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::High,gain);
}

void DlgPrefWaveform::onSetNormalizeOverview(bool normalize) {
    WaveformWidgetFactory::instance()->setOverviewNormalized(normalize);
}

void DlgPrefWaveform::onWaveformMeasured(float frameRate, int droppedFrames) {
    frameRateAverage->setText(
            QString::number((double)frameRate, 'f', 2) + " : " +
            tr("dropped frames") + " " + QString::number(droppedFrames));
}
