#include "previewdeck.h"

PreviewDeck::PreviewDeck(ConfigObject<ConfigValue> *pConfig,
                         EngineMaster* pMixingEngine,
                         EffectsManager* pEffectsManager,
                         EngineChannel::ChannelOrientation defaultOrientation,
                         const QString &group,
                         QObject *pParent=nullptr) :
        BaseTrackPlayerImpl(pConfig, pMixingEngine, pEffectsManager,
                            defaultOrientation, group, false, true,pParent) {
}

PreviewDeck::~PreviewDeck() {
}
