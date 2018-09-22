#include "engine/channelmixer.h"
#include "util/sample.h"
#include "util/timer.h"
////////////////////////////////////////////////////////
// THIS FILE IS AUTO-GENERATED. DO NOT EDIT DIRECTLY! //
// SEE scripts/generate_sample_functions.py           //
////////////////////////////////////////////////////////

// static
void ChannelMixer::applyEffectsAndMixChannels(const EngineMaster::GainCalculator& gainCalculator,
                                              QVarLengthArray<EngineMaster::ChannelInfo*, kPreallocatedChannels>* activeChannels,
                                              QVarLengthArray<EngineMaster::GainCache, kPreallocatedChannels>* channelGainCache,
                                              CSAMPLE* pOutput,
                                              const ChannelHandle& outputHandle,
                                              unsigned int iBufferSize,
                                              unsigned int iSampleRate,
                                              EngineEffectsManager* pEngineEffectsManager) {
    // Signal flow overview:
    // 1. Clear pOutput buffer
    // 2. Calculate gains for each channel
    // 3. Pass each channel's calculated gain and input buffer to pEngineEffectsManager, which then:
    //     A) Copies each channel input buffer to a temporary buffer
    //     B) Applies gain to the temporary buffer
    //     C) Processes effects on the temporary buffer
    //     D) Mixes the temporary buffer into pOutput
    // The original channel input buffers are not modified.
    int totalActive = activeChannels->size();
    SampleUtil::clear(pOutput, iBufferSize);
    for (auto i = 0; i < totalActive; ++i) {
        auto pChannelInfo = activeChannels->at(i);
        auto channelIndex = pChannelInfo->m_index;
        auto& gainCache = (*channelGainCache)[channelIndex];
        auto oldGain = gainCache.m_gain;
        auto newGain = gainCache.m_gain = (gainCache.m_fadeout ? CSAMPLE_GAIN_ZERO : gainCalculator.getGain(pChannelInfo));
        gainCache.m_fadeout = false;
        auto pBuffer = pChannelInfo->m_pBuffer;
        pEngineEffectsManager->processPostFaderAndMix(pChannelInfo->m_handle, outputHandle, pBuffer, pOutput, iBufferSize, iSampleRate, pChannelInfo->m_features, oldGain, newGain);
    }
}
void ChannelMixer::applyEffectsInPlaceAndMixChannels(const EngineMaster::GainCalculator& gainCalculator,
                                                     QVarLengthArray<EngineMaster::ChannelInfo*, kPreallocatedChannels>* activeChannels,
                                                     QVarLengthArray<EngineMaster::GainCache, kPreallocatedChannels>* channelGainCache,
                                                     CSAMPLE* pOutput,
                                                     const ChannelHandle& outputHandle,
                                                     unsigned int iBufferSize,
                                                     unsigned int iSampleRate,
                                                     EngineEffectsManager* pEngineEffectsManager) {
    // Signal flow overview:
    // 1. Calculate gains for each channel
    // 2. Pass each channel's calculated gain and input buffer to pEngineEffectsManager, which then:
    //    A) Applies the calculated gain to the channel buffer, modifying the original input buffer
    //    B) Applies effects to the buffer, modifying the original input buffer
    // 4. Mix the channel buffers together to make pOutput, overwriting the pOutput buffer from the last engine callback
    int totalActive = activeChannels->size();
    ScopedTimer t("EngineMaster::applyEffectsInPlaceAndMixChannels_Over32active");
    SampleUtil::clear(pOutput, iBufferSize);
    for (auto i = 0; i < activeChannels->size(); ++i) {
        auto pChannelInfo = activeChannels->at(i);
        auto channelIndex = pChannelInfo->m_index;
        auto& gainCache = (*channelGainCache)[channelIndex];
        auto oldGain = gainCache.m_gain;
        auto newGain = gainCache.m_gain = (gainCache.m_fadeout ? CSAMPLE_GAIN_ZERO : gainCalculator.getGain(pChannelInfo));
        gainCache.m_fadeout = false;
        auto pBuffer = pChannelInfo->m_pBuffer;
        pEngineEffectsManager->processPostFaderInPlace(pChannelInfo->m_handle, outputHandle, pBuffer, iBufferSize, iSampleRate, pChannelInfo->m_features, oldGain, newGain);
        SampleUtil::addWithGain(pOutput, pBuffer, CSAMPLE_ONE,iBufferSize);
    }
}
