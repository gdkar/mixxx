#ifndef SAMPLER_H
#define SAMPLER_H

#include "basetrackplayer.h"

class Sampler : public BaseTrackPlayerImpl {
    Q_OBJECT
    public:
    Sampler(QObject* pParent,
            QJSEngine *pEngine,
            ConfigObject<ConfigValue> *pConfig,
            EngineMaster* pMixingEngine,
            EffectsManager* pEffectsManager,
            EngineChannel::ChannelOrientation defaultOrientation,
            QString group);
    virtual ~Sampler();
};

#endif
