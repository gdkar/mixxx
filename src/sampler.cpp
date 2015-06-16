#include "sampler.h"

#include "controlobject.h"

Sampler::Sampler(ConfigObject<ConfigValue>* pConfig,
                 EngineMaster* pMixingEngine,
                 EffectsManager* pEffectsManager,
                 EngineChannel::ChannelOrientation defaultOrientation,
                 const QString &group,
                 QObject *pParent=nullptr) :
        BaseTrackPlayerImpl(pConfig, pMixingEngine, pEffectsManager,
                            defaultOrientation, group, true, false,pParent) {
}

Sampler::~Sampler() {
}
