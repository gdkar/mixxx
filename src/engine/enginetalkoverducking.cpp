#include "controlobjectslave.h"
#include "engine/enginetalkoverducking.h"

#define DUCK_THRESHOLD 0.1

EngineTalkoverDucking::EngineTalkoverDucking(const QString &_group,
        ConfigObject<ConfigValue>* pConfig, QObject *pParent)
    : EngineSideChainCompressor(_group, pParent),
      m_group(_group),
      m_pConfig(pConfig){
    m_pMasterSampleRate = new ControlObjectSlave(group(), "samplerate", this);
    m_pMasterSampleRate->connectValueChanged(SLOT(slotSampleRateChanged(double)),
                                             Qt::DirectConnection);

    m_pDuckStrength = new ControlPotmeter(ConfigKey(group(), "duckStrength"), 0.0, 1.0);
    m_pDuckStrength->set(
            m_pConfig->getValueString(ConfigKey(group(), "duckStrength"), "90").toDouble() / 100);
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

    m_pTalkoverDucking = new ControlPushButton(ConfigKey(group(), "talkoverDucking"));
    m_pTalkoverDucking->setButtonMode(ControlPushButton::TOGGLE);
    m_pTalkoverDucking->setStates(3);
    m_pTalkoverDucking->set(
            m_pConfig->getValueString(ConfigKey(group(), "duckMode"), QString::number(AUTO)).toDouble());
    connect(m_pTalkoverDucking, SIGNAL(valueChanged(double)),
            this, SLOT(slotDuckModeChanged(double)),
            Qt::DirectConnection);
}
EngineTalkoverDucking::~EngineTalkoverDucking() {
    m_pConfig->set(ConfigKey(group(), "duckStrength"), ConfigValue(m_pDuckStrength->get() * 100));
    m_pConfig->set(ConfigKey(group(), "duckMode"), ConfigValue(m_pTalkoverDucking->get()));
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
    m_pConfig->set(ConfigKey(group(), "duckStrength"), ConfigValue(strength * 100));
}
void EngineTalkoverDucking::slotDuckModeChanged(double mode) {
   m_pConfig->set(ConfigKey(group(), "duckMode"), ConfigValue(mode));
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
