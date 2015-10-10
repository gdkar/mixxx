
#include "engine/channelmixer.h"
#include "sampleutil.h"
#include <algorithm>
#include <cstring>
void ChannelMixer::mixChannels(
    const EngineMaster::GainCalculator& gainCalculator,
    QVarLengthArray<ChannelInfo*, kPreallocatedChannels>* activeChannels,
    QVarLengthArray<EngineMaster::GainCache, kPreallocatedChannels>* channelGainCache,
    CSAMPLE* pOutput,
    unsigned int iBufferSize)
{
  auto totalActive = activeChannels->size();
  if(!totalActive) std::fill(pOutput,pOutput+iBufferSize,0);
  else{
    auto pChannel     = activeChannels->at(0);
    auto channelIndex = pChannel->m_index;
    auto &gainCache   = (*channelGainCache)[channelIndex];
    auto newGain = CSAMPLE_GAIN{0};
    if(gainCache.m_fadeout)
    {
         newGain = 0;
         gainCache.m_fadeout = false;
    }
    else newGain = gainCalculator.getGain(pChannel);
    gainCache.m_gain = newGain;
    auto pBuffer = &pChannel->m_pBuffer[0];
    SampleUtil::copy1WithGain(pOutput,pBuffer,newGain,iBufferSize);
    for(auto i = 1; i < totalActive;i++)
    {
      pChannel = activeChannels->at(i);
      channelIndex = pChannel->m_index;
      auto &gainCache1 = (*channelGainCache)[channelIndex];
      if(gainCache1.m_fadeout)
      {
        newGain = 0;
        gainCache1.m_fadeout = false;
      }
      else newGain = gainCalculator.getGain(pChannel);
      gainCache1.m_gain = newGain;
      pBuffer = &pChannel->m_pBuffer[0];
      SampleUtil::copy1WithGainAdding(pOutput,pBuffer,newGain,iBufferSize);
    }
  }
}
void ChannelMixer::mixChannelsRamping(
    const EngineMaster::GainCalculator& gainCalculator,
    QVarLengthArray<ChannelInfo*, kPreallocatedChannels>* activeChannels,
    QVarLengthArray<EngineMaster::GainCache, kPreallocatedChannels>* channelGainCache,
    CSAMPLE* pOutput,
    unsigned int iBufferSize)
{
  const int totalActive = activeChannels->size();
  if(!totalActive) std::fill(pOutput,pOutput+iBufferSize,0);
  else{
    auto pChannel     = activeChannels->at(0);
    auto channelIndex = pChannel->m_index;
    auto &gainCache   = (*channelGainCache)[channelIndex];
    auto newGain = CSAMPLE_GAIN{0};
    auto oldGain = gainCache.m_gain;
    if(gainCache.m_fadeout)
    {
      newGain = 0;
      gainCache.m_fadeout = false;
    }
    else newGain = gainCalculator.getGain(pChannel);
    gainCache.m_gain = newGain;
    auto pBuffer = &pChannel->m_pBuffer[0];
    SampleUtil::copy1WithRampingGain(pOutput,pBuffer,oldGain,newGain,iBufferSize);
    for(auto i = 1; i < totalActive;i++)
    {
      pChannel = activeChannels->at(i);
      channelIndex = pChannel->m_index;
      auto &gainCache1 = (*channelGainCache)[channelIndex];
      oldGain = gainCache1.m_gain;
      if(gainCache1.m_fadeout)
      {
            newGain = 0;
            gainCache1.m_fadeout = false;
      }else newGain = gainCalculator.getGain(pChannel);
      gainCache1.m_gain = newGain;
      pBuffer = &pChannel->m_pBuffer[0];
      SampleUtil::copy1WithRampingGainAdding(pOutput,pBuffer,oldGain,newGain,iBufferSize);
    }
  }
}
