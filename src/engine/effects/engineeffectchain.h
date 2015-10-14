_Pragma("once")
#include <QString>
#include <QList>
#include <QLinkedList>

#include "util.h"
#include "util/types.h"
#include "engine/channelhandle.h"
#include "engine/effects/message.h"
#include "engine/effects/groupfeaturestate.h"
#include "effects/effectchain.h"

class EngineEffect;

class EngineEffectChain : public EffectsRequestHandler {
  public:
    EngineEffectChain(const QString& id);
    virtual ~EngineEffectChain();
    bool processEffectsRequest( const EffectsRequest& message, EffectsResponsePipe* pResponsePipe);
    void process(const ChannelHandle& handle,
                 CSAMPLE* pInOut,
                 const unsigned int numSamples,
                 const unsigned int sampleRate,
                 const GroupFeatureState& groupFeatures);
    const QString& id() const { return m_id; }
    bool enabledForChannel(const ChannelHandle& handle) const;
  private:
    struct ChannelStatus {
        CSAMPLE old_gain = 0;
        EffectProcessor::EnableState enable_state = EffectProcessor::DISABLED;
    };
    QString debugString() const { return QString("EngineEffectChain(%1)").arg(m_id); }
    bool updateParameters(const EffectsRequest& message);
    bool addEffect(EngineEffect* pEffect, int iIndex);
    bool removeEffect(EngineEffect* pEffect, int iIndex);
    bool enableForChannel(const ChannelHandle& handle);
    bool disableForChannel(const ChannelHandle& handle);
    // Gets or creates a ChannelStatus entry in m_channelStatus for the provided
    // handle.
    ChannelStatus& getChannelStatus(const ChannelHandle& handle);
    QString m_id;
    EffectProcessor::EnableState m_enableState;
    EffectChain::InsertionType m_insertionType;
    CSAMPLE m_dMix = 0;
    QList<EngineEffect*> m_effects;
    CSAMPLE* m_pBuffer = nullptr;
    ChannelHandleMap<ChannelStatus> m_channelStatus;
};
