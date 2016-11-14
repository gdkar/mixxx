#include "engine/channelmixer.h"
#include "util/sample.h"

void ChannelMixer::mixChannels(
        const EngineMaster::GainCalculator& gcalc,
        QVarLengthArray<EngineMaster::ChannelInfo*, kPreallocatedChannels> *activeChannels,
        QVarLengthArray<EngineMaster::GainCache,   kPreallocatedChannels> *gainCache,
        CSAMPLE *dst,
        unsigned int num)
{
    SampleUtil::clear(dst,num);

    for(auto pChannel : *activeChannels){
        CSAMPLE_GAIN newGain = 0;
        auto &gCache  = (*gainCache)[pChannel->m_index];
        if(!gCache.m_fadeout) {
            newGain = gcalc.getGain(pChannel);
        }
        gCache.m_fadeout = false;
        gCache.m_gain    = newGain;
        if(newGain)
            SampleUtil::addWithGain(dst,pChannel->m_pBuffer, newGain, num);
    }
}
void ChannelMixer::mixChannelsRamping(
        const EngineMaster::GainCalculator& gcalc,
        QVarLengthArray<EngineMaster::ChannelInfo*, kPreallocatedChannels> *activeChannels,
        QVarLengthArray<EngineMaster::GainCache,   kPreallocatedChannels> *gainCache,
        CSAMPLE *dst,
        unsigned int num)
{
    SampleUtil::clear(dst,num);
    for(auto &pChannel : *activeChannels) {
        auto &gCache  = (*gainCache)[pChannel->m_index];
        auto oldGain = gCache.m_gain;
        auto newGain = oldGain;
        if(gCache.m_fadeout) {
            newGain = 0;
            gCache.m_fadeout = false;
        }else{
            newGain = gcalc.getGain(pChannel);
        }
        gCache.m_gain    = newGain;
        if(newGain)
            SampleUtil::addWithRampingGain(dst,pChannel->m_pBuffer, oldGain, newGain, num);
    }
}
