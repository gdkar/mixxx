#ifndef PREVIEWDECK_H
#define PREVIEWDECK_H

#include "basetrackplayer.h"

class PreviewDeck : public BaseTrackPlayerImpl {
    Q_OBJECT
  public:
    PreviewDeck(ConfigObject<ConfigValue> *pConfig,
                EngineMaster* pMixingEngine,
                EffectsManager* pEffectsManager,
                EngineChannel::ChannelOrientation defaultOrientation,
                const QString &group,
                QObject *pParent=nullptr);
    virtual ~PreviewDeck();
};

#endif /* PREVIEWDECK_H */
