#include "deck.h"

Deck::Deck(ConfigObject<ConfigValue>* pConfig,
           EngineMaster* pMixingEngine,
           EffectsManager* pEffectsManager,
           EngineChannel::ChannelOrientation defaultOrientation,
           const QString &group,
           QObject *pParent) :
        BaseTrackPlayerImpl(pConfig, pMixingEngine, pEffectsManager,
                            defaultOrientation, group, true, false,pParent) {
}

Deck::~Deck() {
}
