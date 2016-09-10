#include "mixer/microphone.h"

#include "control/controlproxy.h"
#include "engine/enginemaster.h"
#include "engine/enginemicrophone.h"
#include "soundio/soundmanager.h"
#include "soundio/soundmanagerutil.h"

Microphone::Microphone(QObject* pParent, const QString& group, int index,
                       SoundManager* pSoundManager, EngineMaster* pEngine,
                       EffectsManager* pEffectsManager)
        : BasePlayer(pParent, group) {
    auto channelGroup = pEngine->registerChannelGroup(group);
    auto  pMicrophone =
            new EngineMicrophone(pEngine, channelGroup, pEffectsManager);
    pEngine->addChannel(pMicrophone);
    auto micInput = AudioInput(AudioPath::MICROPHONE, 0, 2, index);
    pSoundManager->registerInput(micInput, pMicrophone);

    m_pInputConfigured.reset(new ControlProxy(group, "input_configured", this));
    m_pTalkoverEnabled.reset(new ControlProxy(group, "talkover", this));
    m_pTalkoverEnabled->connectValueChanged(SLOT(slotTalkoverEnabled(double)));
}

Microphone::~Microphone() = default;

void Microphone::slotTalkoverEnabled(double v)
{
    auto configured = m_pInputConfigured->toBool();
    auto talkover = v > 0.0;

    // Warn the user if they try to enable talkover on a microphone with no
    // configured input.
    if (!configured && talkover) {
        m_pTalkoverEnabled->set(0.0);
        emit(noMicrophoneInputConfigured());
    }
}
