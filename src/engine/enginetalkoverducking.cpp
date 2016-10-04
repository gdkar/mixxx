#include "control/controlpotmeter.h"
#include "control/controlpushbutton.h"
#include "control/controlproxy.h"
#include "control/controlobject.h"
#include "engine/enginetalkoverducking.h"

#define DUCK_THRESHOLD 0.1

EngineTalkoverDucking::EngineTalkoverDucking(
        UserSettingsPointer pConfig, const char* group)
    : EngineSideChainCompressor(group),
      m_pConfig(pConfig),
      m_group(group) {
    m_pMasterSampleRate = new ControlProxy(m_group, "samplerate", this);
    m_pMasterSampleRate->connectValueChanged(SLOT(slotSampleRateChanged(double)),
                                             Qt::DirectConnection);

    m_pDuckStrength = new ControlPotmeter(ConfigKey(m_group, "duckStrength"),this, 0.0, 1.0);
    m_pDuckStrength->set(
            m_pConfig->getValueString(ConfigKey(m_group, "duckStrength"), "90").toDouble() / 100);
    connect(m_pDuckStrength, SIGNAL(valueChanged(double)),
            this, SLOT(slotDuckStrengthChanged(double)),
            Qt::DirectConnection);

    // We only allow the strength to be configurable for now.  The next most likely
    // candidate for customization is the threshold, which may be too low for
    // noisy club situations.
    setParameters(
            DUCK_THRESHOLD,
            m_pDuckStrength->get(),
            m_pMasterSampleRate->get() / 2 * .1,
            m_pMasterSampleRate->get() / 2);
    auto button = new ControlPushButton(ConfigKey(m_group, "talkoverDucking"),this);
    m_pTalkoverDucking = button;
    button->setButtonMode(ControlPushButton::TOGGLE);
    button->setStates(3);
    m_pTalkoverDucking->set(
            m_pConfig->getValueString(
                ConfigKey(m_group, "duckMode"), QString::number(AUTO)).toDouble());
    connect(m_pTalkoverDucking, SIGNAL(valueChanged(double)),
            this, SLOT(slotDuckModeChanged(double)),
            Qt::DirectConnection);
}

EngineTalkoverDucking::~EngineTalkoverDucking() {
    m_pConfig->set(ConfigKey(m_group, "duckStrength"), ConfigValue(m_pDuckStrength->get() * 100));
    m_pConfig->set(ConfigKey(m_group, "duckMode"), ConfigValue(m_pTalkoverDucking->get()));

    delete m_pDuckStrength;
    delete m_pTalkoverDucking;
}

void EngineTalkoverDucking::slotSampleRateChanged(double samplerate) {
    setParameters(
            DUCK_THRESHOLD, m_pDuckStrength->get(),
            samplerate / 2 * .1, samplerate / 2);
}

void EngineTalkoverDucking::slotDuckStrengthChanged(double strength) {
    setParameters(
            DUCK_THRESHOLD, strength,
            m_pMasterSampleRate->get() / 2 * .1, m_pMasterSampleRate->get() / 2);
    m_pConfig->set(ConfigKey(m_group, "duckStrength"), ConfigValue(strength * 100));
}

void EngineTalkoverDucking::slotDuckModeChanged(double mode) {
   m_pConfig->set(ConfigKey(m_group, "duckMode"), ConfigValue(mode));
}

CSAMPLE EngineTalkoverDucking::getGain(int numFrames) {
    // Apply microphone ducking.
    switch (getMode()) {
      case EngineTalkoverDucking::OFF:
        return 1.0;
      case EngineTalkoverDucking::AUTO:
        return calculateCompressedGain(numFrames);
      case EngineTalkoverDucking::MANUAL:
        return m_pDuckStrength->get();
    }
    qWarning() << "Invalid ducking mode, returning 1.0";
    return 1.0;
}
EngineTalkoverDucking::TalkoverDuckSetting EngineTalkoverDucking::getMode() const {
    return static_cast<TalkoverDuckSetting>(int(m_pTalkoverDucking->get()));
}

