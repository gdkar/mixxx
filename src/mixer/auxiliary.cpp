#include "mixer/auxiliary.h"

#include "engine/engineaux.h"
#include "engine/enginemaster.h"
#include "soundio/soundmanager.h"
#include "soundio/soundmanagerutil.h"

Auxiliary::Auxiliary(QObject* pParent, const QString& group, int index,
                     SoundManager* pSoundManager, EngineMaster* pEngine,
                     EffectsManager* pEffectsManager)
        : BasePlayer(pParent, group) {
    auto channelGroup = pEngine->registerChannelGroup(group);
    auto  pAuxiliary = new EngineAux(pEngine, channelGroup, pEffectsManager);
    pEngine->addChannel(pAuxiliary);
    auto auxInput = AudioInput(AudioPath::AUXILIARY, 0, 2, index);
    pSoundManager->registerInput(auxInput, pAuxiliary);
}

Auxiliary::~Auxiliary() {
}
