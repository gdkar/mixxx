#include "dialogs/dlgprefreplaygain.h"

#include "control/controlobject.h"
#include "util/math.h"

#define kConfigKey "[ReplayGain]"

static const int kReplayGainReferenceLUFS = -18; 


DlgPrefReplayGain::DlgPrefReplayGain(QWidget * parent, ConfigObject<ConfigValue> * _config)
        : DlgPreferencePage(parent),
          config(_config),
          m_replayGainBoost(kConfigKey, "ReplayGainBoost"),
          m_defaultBoost(kConfigKey, "DefaultBoost"),
          m_enabled(kConfigKey, "ReplayGainEnabled") {
    setupUi(this);

    //Connections
    connect(EnableGain, SIGNAL(stateChanged(int)),
            this, SLOT(onSetRGEnabled()));
    connect(EnableAnalyser, SIGNAL(stateChanged(int)),
            this, SLOT(onSetRGAnalyserEnabled()));
    connect(SliderReplayGainBoost, SIGNAL(valueChanged(int)),
            this, SLOT(onUpdateReplayGainBoost()));
    connect(SliderReplayGainBoost, SIGNAL(sliderReleased()),
            this, SLOT(onApply()));
    connect(SliderDefaultBoost, SIGNAL(valueChanged(int)),
            this, SLOT(onUpdateDefaultBoost()));
    connect(SliderDefaultBoost, SIGNAL(sliderReleased()),
            this, SLOT(onApply()));

    loadSettings();
}

DlgPrefReplayGain::~DlgPrefReplayGain() {
}

void DlgPrefReplayGain::loadSettings() {
    int iReplayGainBoost = config->getValueString(
            ConfigKey(kConfigKey, "InitialReplayGainBoost"), "0").toInt();
    SliderReplayGainBoost->setValue(iReplayGainBoost);
    setLabelCurrentReplayGainBoost(iReplayGainBoost);


    int iDefaultBoost = config->getValueString(
            ConfigKey(kConfigKey, "InitialDefaultBoost"), "-6").toInt();
    SliderDefaultBoost->setValue(iDefaultBoost);
    LabelCurrentDefaultBoost->setText(
            QString("%1 dB").arg(iDefaultBoost));

    bool gainEnabled = config->getValueString(
            ConfigKey(kConfigKey, "ReplayGainEnabled"), "1").toInt() == 1;
    EnableGain->setChecked(gainEnabled);

    bool analyserEnabled = config->getValueString(
            ConfigKey(kConfigKey, "ReplayGainAnalyserEnabled"), "1").toInt();
    EnableAnalyser->setChecked(analyserEnabled);

    onUpdate();
    onUpdateReplayGainBoost();
    onUpdateDefaultBoost();
}

void DlgPrefReplayGain::onResetToDefaults() {
    EnableGain->setChecked(true);
    // Turn ReplayGain Analyser on by default as it does not give appreciable
    // delay on recent hardware (<5 years old).
    EnableAnalyser->setChecked(true);
    SliderReplayGainBoost->setValue(0);
    setLabelCurrentReplayGainBoost(0);
    SliderDefaultBoost->setValue(-6);
    LabelCurrentDefaultBoost->setText("-6 dB");

    int iDefaultBoost = config->getValueString(
            ConfigKey(kConfigKey, "InitialDefaultBoost"), "-6").toInt();
    SliderDefaultBoost->setValue(iDefaultBoost);
    LabelCurrentDefaultBoost->setText(
            QString("%1 dB").arg(iDefaultBoost));


    onUpdate();
    onApply();
}

void DlgPrefReplayGain::onSetRGEnabled() {
    if (EnableGain->isChecked()) {
        config->set(ConfigKey(kConfigKey,"ReplayGainEnabled"), ConfigValue(1));
    } else {
        config->set(ConfigKey(kConfigKey,"ReplayGainEnabled"), ConfigValue(0));
    }
    onUpdate();
    onApply();
}

void DlgPrefReplayGain::onSetRGAnalyserEnabled() {
    int enabled = EnableAnalyser->isChecked() ? 1 : 0;
    config->set(ConfigKey(kConfigKey,"ReplayGainAnalyserEnabled"),
                ConfigValue(enabled));
    onApply();
}

void DlgPrefReplayGain::onUpdateReplayGainBoost() {
    int value = SliderReplayGainBoost->value();
    config->set(ConfigKey(kConfigKey, "InitialReplayGainBoost"),
                ConfigValue(value));
    setLabelCurrentReplayGainBoost(value);
    onApply();
}

void DlgPrefReplayGain::setLabelCurrentReplayGainBoost(int value) {
    LabelCurrentReplayGainBoost->setText(
            QString(tr("%1 LUFS (adjust by %2 dB)")).arg(
                  QString::number(value + kReplayGainReferenceLUFS), QString().sprintf("%+d", value)));
}

void DlgPrefReplayGain::onUpdateDefaultBoost() {
    int value = SliderDefaultBoost->value();
    config->set(ConfigKey(kConfigKey, "InitialDefaultBoost"),
                ConfigValue(value));
    LabelCurrentDefaultBoost->setText(
            QString("%1 dB").arg(value));
    onApply();
}

void DlgPrefReplayGain::onUpdate() {
    if (config->getValueString(
            ConfigKey(kConfigKey,"ReplayGainEnabled")).toInt() == 1) {
        SliderReplayGainBoost->setEnabled(true);
        SliderDefaultBoost->setEnabled(true);
    } else {
        SliderReplayGainBoost->setEnabled(false);
        SliderDefaultBoost->setEnabled(false);
    }
}

void DlgPrefReplayGain::onApply() {
    double replayGainBoostDb = SliderReplayGainBoost->value();
    m_replayGainBoost.set(db2ratio(replayGainBoostDb));
    double defaultBoostDb = SliderDefaultBoost->value();
    m_defaultBoost.set(db2ratio(defaultBoostDb));
    m_enabled.set(EnableGain->isChecked() ? 1.0 : 0.0);
}
