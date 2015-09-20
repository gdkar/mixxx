
#include "engine/channelmixer.h"
#include "sampleutil.h"
#include <cstring>
void ChannelMixer::mixChannels(
    const EngineMaster::GainCalculator& gainCalculator,
    QVarLengthArray<EngineMaster::ChannelInfo*, kPreallocatedChannels>* activeChannels,
    QVarLengthArray<EngineMaster::GainCache, kPreallocatedChannels>* channelGainCache,
    CSAMPLE* pOutput,
    unsigned int iBufferSize)
{
  const int totalActive = activeChannels->size();
  if(!totalActive) std::memset(pOutput,0,sizeof(CSAMPLE)*iBufferSize);
  else{
    auto pChannel     = activeChannels->at(0);
    auto channelIndex = pChannel->m_index;
    auto &gainCache   = (*channelGainCache)[channelIndex];
    CSAMPLE_GAIN newGain;
    if(gainCache.m_fadeout){
      newGain = 0;
      gainCache.m_fadeout = false;
    }else{
      newGain = gainCalculator.getGain(pChannel);
    }
    gainCache.m_gain = newGain;
    auto pBuffer = pChannel->m_pBuffer;
    SampleUtil::copy1WithGain(pOutput,pBuffer,newGain,iBufferSize);
    for(auto i = 1; i < totalActive;i++)
    {
      pChannel = activeChannels->at(i);
      channelIndex = pChannel->m_index;
      auto &gainCache1 = (*channelGainCache)[channelIndex];
      if(gainCache1.m_fadeout){
        newGain = 0;
        gainCache1.m_fadeout = false;
      }else{
        newGain = gainCalculator.getGain(pChannel);
      }
      gainCache1.m_gain = newGain;
      pBuffer = pChannel->m_pBuffer;
      SampleUtil::copy1WithGainAdding(pOutput,pBuffer,newGain,iBufferSize);
    }
  }
}
void ChannelMixer::mixChannelsRamping(
    const EngineMaster::GainCalculator& gainCalculator,
    QVarLengthArray<EngineMaster::ChannelInfo*, kPreallocatedChannels>* activeChannels,
    QVarLengthArray<EngineMaster::GainCache, kPreallocatedChannels>* channelGainCache,
    CSAMPLE* pOutput,
    unsigned int iBufferSize)
{
  const int totalActive = activeChannels->size();
  if(!totalActive) std::memset(pOutput,0,sizeof(CSAMPLE)*iBufferSize);
  else{
    auto pChannel     = activeChannels->at(0);
    auto channelIndex = pChannel->m_index;
    auto &gainCache   = (*channelGainCache)[channelIndex];
    CSAMPLE_GAIN oldGain,newGain;
    oldGain = gainCache.m_gain;
    if(gainCache.m_fadeout){
      newGain = 0;
      gainCache.m_fadeout = false;
    }else{
      newGain = gainCalculator.getGain(pChannel);
    }
    gainCache.m_gain = newGain;
    auto pBuffer = pChannel->m_pBuffer;
    SampleUtil::copy1WithRampingGain(pOutput,pBuffer,oldGain,newGain,iBufferSize);
    for(auto i = 1; i < totalActive;i++)
    {
      pChannel = activeChannels->at(i);
      channelIndex = pChannel->m_index;
      auto &gainCache1 = (*channelGainCache)[channelIndex];
      oldGain = gainCache1.m_gain;
      if(gainCache1.m_fadeout){
        newGain = 0;
        gainCache1.m_fadeout = false;
      }else{
        newGain = gainCalculator.getGain(pChannel);
      }
      gainCache1.m_gain = newGain;
      pBuffer = pChannel->m_pBuffer;
      SampleUtil::copy1WithRampingGainAdding(pOutput,pBuffer,oldGain,newGain,iBufferSize);
    }
  }
}
