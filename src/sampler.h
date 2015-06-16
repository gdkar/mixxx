#ifndef SAMPLER_H
#define SAMPLER_H

#include "basetrackplayer.h"

class Sampler : public BaseTrackPlayerImpl {
    Q_OBJECT
    public:
    Sampler(ConfigObject<ConfigValue> *pConfig,
            EngineMaster* pMixingEngine,
            EffectsManager* pEffectsManager,
            EngineChannel::ChannelOrientation defaultOrientation,
            const QString &group,
            QObject *pParent=nullptr);
    virtual ~Sampler();
};

#endif
