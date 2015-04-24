#include "previewdeck.h"

PreviewDeck::PreviewDeck(QObject* pParent,
                         QJSEngine* pEngine,
                         ConfigObject<ConfigValue> *pConfig,
                         EngineMaster* pMixingEngine,
                         EffectsManager* pEffectsManager,
                         EngineChannel::ChannelOrientation defaultOrientation,
                         QString group) :
        BaseTrackPlayerImpl(pParent, pEngine,pConfig, pMixingEngine, pEffectsManager,
                            defaultOrientation, group, false, true) {
}

PreviewDeck::~PreviewDeck() {
}
