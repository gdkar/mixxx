#ifndef CHANNELMIXER_H
#define CHANNELMIXER_H

#include <QVarLengthArray>

#include "util/types.h"
#include "engine/enginemaster.h"

namespace ChannelMixer {
    void mixChannels(
        const EngineMaster::GainCalculator& gainCalculator,
        QVarLengthArray<EngineMaster::ChannelInfo*, kPreallocatedChannels>* activeChannels,
        QVarLengthArray<EngineMaster::GainCache, kPreallocatedChannels>* channelGainCache,
        CSAMPLE* pOutput,
        unsigned int iBufferSize);

    void mixChannelsRamping(
        const EngineMaster::GainCalculator& gainCalculator,
        QVarLengthArray<EngineMaster::ChannelInfo*, kPreallocatedChannels>* activeChannels,
        QVarLengthArray<EngineMaster::GainCache, kPreallocatedChannels>* channelGainCache,
        CSAMPLE* pOutput,
        unsigned int iBufferSize);
};

#endif /* CHANNELMIXER_H */
