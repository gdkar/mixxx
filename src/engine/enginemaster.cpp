
#include <QtDebug>
#include <QList>
#include <QPair>

#include "control/control.h"
#include "controlpushbutton.h"
#include "configobject.h"
#include "controlaudiotaperpot.h"
#include "controlpotmeter.h"
#include "controlaudiotaperpot.h"
#include "engine/enginebuffer.h"
#include "engine/enginemaster.h"
#include "engine/engineworkerscheduler.h"
#include "engine/enginedeck.h"
#include "engine/enginebuffer.h"
#include "engine/enginechannel.h"
#include "engine/enginetalkoverducking.h"
#include "engine/enginevumeter.h"
#include "engine/enginexfader.h"
#include "engine/enginedelay.h"
#include "engine/sidechain/enginesidechain.h"
#include "engine/sync/enginesync.h"
#include "sampleutil.h"
#include "engine/effects/engineeffectsmanager.h"
#include "effects/effectsmanager.h"
#include "util/timer.h"
#include "util/trace.h"
#include "util/defs.h"
#include "playermanager.h"
#include "engine/channelmixer.h"
ChannelInfo::ChannelInfo(ChannelHandle handle, int index, EngineChannel * pChannel, QObject *pParent)
  : QObject(pParent),
    m_handle(handle),
    m_pChannel(pChannel),
    m_pBuffer(std::make_unique<CSAMPLE[]>(MAX_BUFFER_LEN)),
    m_index(index)
{
  if(pChannel)
  {
    auto group = pChannel->getGroup();
    m_pVolumeControl = new ControlAudioTaperPot(ConfigKey(group, "volume"), -20, 0, 1);
    m_pVolumeControl->setParent(this);
    m_pVolumeControl->setDefaultValue(1.0);
    m_pVolumeControl->set(1.0);
    m_pMuteControl = new ControlPushButton(ConfigKey(group, "mute"),false,this);
    m_pMuteControl->setButtonMode(ControlPushButton::POWERWINDOW);
  }
}
ChannelInfo::ChannelInfo(QObject *pParent )
:ChannelInfo(ChannelHandle{},-1,nullptr,pParent)
{
}
ChannelInfo::~ChannelInfo() = default;
EngineMaster::EngineMaster(ConfigObject<ConfigValue>* _config,
                           const char* group,
                           EffectsManager* pEffectsManager,
                           bool bEnableSidechain,
                           bool bRampingGain)
        : m_pEngineEffectsManager(pEffectsManager ? pEffectsManager->getEngineEffectsManager() : nullptr),
          m_bRampingGain(bRampingGain),
          m_masterGainOld(0.0),
          m_headphoneMasterGainOld(0.0),
          m_headphoneGainOld(1.0),
          m_masterHandle(registerChannelGroup("[Master]")),
          m_headphoneHandle(registerChannelGroup("[Headphone]")),
          m_busLeftHandle(registerChannelGroup("[BusLeft]")),
          m_busCenterHandle(registerChannelGroup("[BusCenter]")),
          m_busRightHandle(registerChannelGroup("[BusRight]")) {
    m_bBusOutputConnected[EngineChannel::LEFT] = false;
    m_bBusOutputConnected[EngineChannel::CENTER] = false;
    m_bBusOutputConnected[EngineChannel::RIGHT] = false;
    m_pWorkerScheduler = new EngineWorkerScheduler(this);
    m_pWorkerScheduler->start(QThread::HighPriority);
    if (pEffectsManager) {
        pEffectsManager->registerChannel(m_masterHandle);
        pEffectsManager->registerChannel(m_headphoneHandle);
        pEffectsManager->registerChannel(m_busLeftHandle);
        pEffectsManager->registerChannel(m_busCenterHandle);
        pEffectsManager->registerChannel(m_busRightHandle);
    }
    // Master sample rate
    m_pMasterSampleRate = new ControlObject(ConfigKey(group, "samplerate"), true, true);
    m_pMasterSampleRate->setParent(this);
    m_pMasterSampleRate->set(44100.);
    // Latency control
    m_pMasterLatency = new ControlObject(ConfigKey(group, "latency"), true, true);
    m_pMasterLatency->setParent(this);
    m_pMasterAudioBufferSize = new ControlObject(ConfigKey(group, "audio_buffer_size"));
    m_pMasterAudioBufferSize->setParent(this);
    m_pAudioLatencyOverloadCount = new ControlObject(ConfigKey(group, "audio_latency_overload_count"), true, true);
    m_pAudioLatencyOverloadCount->setParent(this);
    m_pAudioLatencyUsage = new ControlPotmeter(ConfigKey(group, "audio_latency_usage"), 0.0, 0.25);
    m_pAudioLatencyUsage->setParent(this);
    m_pAudioLatencyOverload  = new ControlPotmeter(ConfigKey(group, "audio_latency_overload"), 0.0, 1.0);
    m_pAudioLatencyOverload->setParent(this);
    // Master rate
    m_pMasterRate = new ControlPotmeter(ConfigKey(group, "rate"), -1.0, 1.0);
    m_pMasterRate->setParent(this);
    // Master sync controller
    m_pMasterSync = new EngineSync(_config);
    // The last-used bpm value is saved in the destructor of EngineSync.
    auto default_bpm = _config->getValueString(ConfigKey("[InternalClock]", "bpm"),"124.0").toDouble();
    ControlObject::set(ConfigKey("[InternalClock","bpm"),default_bpm);
    // Crossfader
    m_pCrossfader = new ControlPotmeter(ConfigKey(group, "crossfader"), -1., 1.);
    m_pCrossfader->setParent(this);
    // Balance
    m_pBalance = new ControlPotmeter(ConfigKey(group, "balance"), -1., 1.);
    m_pBalance->setParent(this);
    // Master gain
    m_pMasterGain = new ControlAudioTaperPot(ConfigKey(group, "gain"), -14, 14, 0.5);
    m_pMasterGain->setParent(this);
    // Legacy: the master "gain" control used to be named "volume" in Mixxx
    // 1.11.0 and earlier. See Bug #1306253.
    ControlDoublePrivate::insertAlias(ConfigKey(group, "volume"), ConfigKey(group, "gain"));
    // VU meter:
    m_pVumeter = new EngineVuMeter(group,this);
    m_pMasterDelay = new EngineDelay(group, ConfigKey(group, "delay"),this);
    m_pHeadDelay = new EngineDelay(group, ConfigKey(group, "headDelay"),this);
    // Headphone volume
    m_pHeadGain = new ControlAudioTaperPot(ConfigKey(group, "headGain"), -14, 14, 0.5);
    m_pHeadGain->setParent(this);
    // Legacy: the headphone "headGain" control used to be named "headVolume" in
    // Mixxx 1.11.0 and earlier. See Bug #1306253.
    ControlDoublePrivate::insertAlias(ConfigKey(group, "headVolume"),ConfigKey(group, "headGain"));
    // Headphone mix (left/right)
    m_pHeadMix = new ControlPotmeter(ConfigKey(group, "headMix"),-1.,1.);
    m_pHeadMix->setParent(this);
    m_pHeadMix->setDefaultValue(-1.);
    m_pHeadMix->set(-1.);
    // Master / Headphone split-out mode (for devices with only one output).
    m_pHeadSplitEnabled = new ControlPushButton(ConfigKey(group, "headSplit"),false,this);
    m_pHeadSplitEnabled->setButtonMode(ControlPushButton::TOGGLE);
    m_pHeadSplitEnabled->set(0.0);
    m_pTalkoverDucking = new EngineTalkoverDucking(_config, group,this);
    // Allocate buffers
    m_pHead = SampleUtil::alloc(MAX_BUFFER_LEN);
    m_pMaster = SampleUtil::alloc(MAX_BUFFER_LEN);
    m_pTalkover = SampleUtil::alloc(MAX_BUFFER_LEN);
    SampleUtil::clear(m_pHead, MAX_BUFFER_LEN);
    SampleUtil::clear(m_pMaster, MAX_BUFFER_LEN);
    SampleUtil::clear(m_pTalkover, MAX_BUFFER_LEN);
    // Setup the output buses
    for (auto o = static_cast<int>(EngineChannel::LEFT); o <= static_cast<int>(EngineChannel::RIGHT); ++o)
    {
        m_pOutputBusBuffers[o] = SampleUtil::alloc(MAX_BUFFER_LEN);
        SampleUtil::clear(m_pOutputBusBuffers[o], MAX_BUFFER_LEN);
    }
    // Starts a thread for recording and shoutcast
    m_pSideChain = bEnableSidechain ? new EngineSideChain(_config,this) : nullptr ;
    // X-Fader Setup
    m_pXFaderMode = new ControlPushButton( ConfigKey("[Mixer Profile]", "xFaderMode"),false,this);
    m_pXFaderMode->setButtonMode(ControlPushButton::TOGGLE);
    m_pXFaderCurve = new ControlPotmeter( ConfigKey("[Mixer Profile]", "xFaderCurve"), 0., 2.);
    m_pXFaderCurve->setParent(this);
    m_pXFaderCalibration = new ControlPotmeter( ConfigKey("[Mixer Profile]", "xFaderCalibration"), -2., 2.);
    m_pXFaderCalibration->setParent(this);
    m_pXFaderReverse = new ControlPushButton(ConfigKey("[Mixer Profile]", "xFaderReverse"),false,this);
    m_pXFaderReverse->setButtonMode(ControlPushButton::TOGGLE);
    m_pKeylockEngine = new ControlObject(ConfigKey(group, "keylock_engine"),true, false, true);
    m_pKeylockEngine->setParent(this);
    m_pKeylockEngine->set(_config->getValueString(ConfigKey(group, "keylock_engine")).toDouble());
    m_pMasterEnabled = new ControlObject(ConfigKey(group, "enabled"),true, false, true);  // persist = true
    m_pMasterEnabled->setParent(this);
    m_pMasterMonoMixdown = new ControlObject(ConfigKey(group, "mono_mixdown"),true, false, true);  // persist = true
    m_pMasterMonoMixdown->setParent(this);
    m_pMasterTalkoverMix = new ControlObject(ConfigKey(group, "talkover_mix"),true, false, true);  // persist = true
    m_pMasterTalkoverMix->setParent(this);
    m_pHeadphoneEnabled = new ControlObject(ConfigKey(group, "headEnabled"),this);
    // Note: the EQ Rack is set in EffectsManager::setupDefaults();
}
EngineMaster::~EngineMaster() {
    qDebug() << "in ~EngineMaster()";
    SampleUtil::free(m_pHead);
    SampleUtil::free(m_pMaster);
    SampleUtil::free(m_pTalkover);
    for (auto o = static_cast<int>(EngineChannel::LEFT); o <= static_cast<int>(EngineChannel::RIGHT); o++) SampleUtil::free(m_pOutputBusBuffers[o]);
    delete m_pWorkerScheduler;
    delete m_pMasterSync;
}
const CSAMPLE* EngineMaster::getMasterBuffer() const 
{
  return m_pMaster;
}
const CSAMPLE* EngineMaster::getHeadphoneBuffer() const 
{
  return m_pHead;
}
void EngineMaster::processChannels(int iBufferSize)
{
    m_activeBusChannels[EngineChannel::LEFT].clear();
    m_activeBusChannels[EngineChannel::CENTER].clear();
    m_activeBusChannels[EngineChannel::RIGHT].clear();
    m_activeHeadphoneChannels.clear();
    m_activeTalkoverChannels.clear();
    m_activeChannels.clear();
    ScopedTimer timer("EngineMaster::processChannels");
    auto pMasterChannel = m_pMasterSync->getMaster();
    // Reserve the first place for the master channel which
    // should be processed first
    m_activeChannels.append(nullptr);
    auto activeChannelsStartIndex = 1; // Nothing at 0 yet
    for (auto i = 0; i < m_channels.size(); ++i)
    {
        auto pChannelInfo = m_channels[i];
        auto pChannel = pChannelInfo->m_pChannel;
        // Skip inactive channels.
        if (!pChannel || !pChannel->isActive()) continue;
        if (pChannel->isTalkoverEnabled())
        {
            // talkover is an exclusive channel
            // once talkover is enabled it is not used in
            // xFader-Mix
            m_activeTalkoverChannels.append(pChannelInfo);
            // Check if we need to fade out the master channel
            auto& gainCache = m_channelMasterGainCache[i];
            if (gainCache.m_gain)
            {
                gainCache.m_fadeout = true;
                m_activeBusChannels[pChannel->getOrientation()].append(pChannelInfo);
             }
        }
        else
        {
            // Check if we need to fade out the channel
            auto& gainCache = m_channelTalkoverGainCache[i];
            if (gainCache.m_gain)
            {
                gainCache.m_fadeout = true;
                m_activeTalkoverChannels.append(pChannelInfo);
            }
            if (pChannel->isMasterEnabled() && !pChannelInfo->m_pMuteControl->toBool())
            {
                // the xFader-Mix
                m_activeBusChannels[pChannel->getOrientation()].append(pChannelInfo);
            }
            else
            {
                // Check if we need to fade out the channel
                auto& gainCache = m_channelMasterGainCache[i];
                if (gainCache.m_gain)
                {
                    gainCache.m_fadeout = true;
                    m_activeBusChannels[pChannel->getOrientation()].append(pChannelInfo);
                }
            }
        }
        // If the channel is enabled for previewing in headphones, copy it
        // over to the headphone buffer
        if (pChannel->isPflEnabled()) m_activeHeadphoneChannels.append(pChannelInfo);
        else
        {
            // Check if we need to fade out the channel
            auto& gainCache = m_channelHeadphoneGainCache[i];
            if (gainCache.m_gain)
            {
                m_channelHeadphoneGainCache[i].m_fadeout = true;
                m_activeHeadphoneChannels.append(pChannelInfo);
            }
        }
        // If necessary, add the channel to the list of buffers to process.
        if (pChannel == pMasterChannel)
        {
            // If this is the sync master, it should be processed first.
            m_activeChannels.replace(0, pChannelInfo);
            activeChannelsStartIndex = 0;
        } else m_activeChannels.append(pChannelInfo);
    }
    // Now that the list is built and ordered, do the processing.
    for(auto &pChannelInfo : m_activeChannels)
    {
        pChannelInfo->m_pChannel->process(&pChannelInfo->m_pBuffer[0],iBufferSize);
    }
    // After all the engines have been processed, trigger post-processing
    // which ensures that all channels are updating certain values at the
    // same point in time.  This prevents sync from failing depending on
    // if the sync target was processed before or after the sync origin.
    for(auto activeChannel : m_activeChannels){
      if(activeChannel && activeChannel->m_pChannel) 
        activeChannel->m_pChannel->postProcess(iBufferSize);
    }
}
void EngineMaster::process(const int iBufferSize) {
    QThread::currentThread()->setObjectName("Engine");
    Trace t("EngineMaster::process");
    auto masterEnabled = m_pMasterEnabled->get();
    auto headphoneEnabled = m_pHeadphoneEnabled->get();
    auto iSampleRate = static_cast<int>(m_pMasterSampleRate->get());
    if (m_pEngineEffectsManager) {m_pEngineEffectsManager->onCallbackStart();}
    // Update internal master sync rate.
    m_pMasterSync->onCallbackStart(iSampleRate, iBufferSize);
    // Prepare each channel for output
    processChannels(iBufferSize);
    // Do internal master sync post-processing
    m_pMasterSync->onCallbackEnd(iSampleRate, iBufferSize);
    // Compute headphone mix
    // Head phone left/right mix
    auto chead_gain   = CSAMPLE{1};
    auto cmaster_gain = CSAMPLE{0};
    if (masterEnabled)
    {
        auto cf_val  = m_pHeadMix->get();
        chead_gain   = 0.5f * (-cf_val + 1);
        cmaster_gain = 0.5f * ( cf_val + 1);
    }
    // Mix all the PFL enabled channels together.
    m_headphoneGain.setGain(chead_gain);
    ChannelMixer::mixChannelsRamping(m_headphoneGain, &m_activeHeadphoneChannels,&m_channelHeadphoneGainCache,m_pHead, iBufferSize);
    // Mix all the talkover enabled channels together.
    ChannelMixer::mixChannelsRamping(m_talkoverGain, &m_activeTalkoverChannels,&m_channelTalkoverGainCache,m_pTalkover, iBufferSize);
    // Clear talkover compressor for the next round of gain calculation.
    m_pTalkoverDucking->clearKeys();
    if (m_pTalkoverDucking->getMode() != EngineTalkoverDucking::OFF) m_pTalkoverDucking->processKey(m_pTalkover, iBufferSize);
    // Calculate the crossfader gains for left and right side of the crossfader
    auto c1_gain = 0.0, c2_gain = 0.0;
    EngineXfader::getXfadeGains(m_pCrossfader->get(), m_pXFaderCurve->get(),
                                m_pXFaderCalibration->get(),
                                m_pXFaderMode->get() == MIXXX_XFADER_CONSTPWR,
                                m_pXFaderReverse->toBool(),
                                &c1_gain, &c2_gain);
    // All other channels should be adjusted by ducking gain.
    // The talkover channels are mixed in later
    m_masterGain.setGains(m_pTalkoverDucking->getGain(iBufferSize / 2),c1_gain, 1.0, c2_gain);
    // Make the mix for each output bus. m_masterGain takes care of applying the
    // master volume, the channel volume, and the orientation gain.
    for (auto o = static_cast<int>(EngineChannel::LEFT); o <= static_cast<int>(EngineChannel::RIGHT); o++)
    {
        ChannelMixer::mixChannelsRamping(
                m_masterGain,
                &m_activeBusChannels[o],
                &m_channelMasterGainCache, // no [o] because the old gain follows an orientation switch
                m_pOutputBusBuffers[o], iBufferSize);
    }
    // Process master channel effects
    if (m_pEngineEffectsManager) {
        GroupFeatureState busFeatures;
        m_pEngineEffectsManager->process(m_busLeftHandle.handle(),m_pOutputBusBuffers[EngineChannel::LEFT],iBufferSize, iSampleRate, busFeatures);
        m_pEngineEffectsManager->process(m_busCenterHandle.handle(),m_pOutputBusBuffers[EngineChannel::CENTER],iBufferSize, iSampleRate, busFeatures);
        m_pEngineEffectsManager->process(m_busRightHandle.handle(),m_pOutputBusBuffers[EngineChannel::RIGHT],iBufferSize, iSampleRate, busFeatures);
    }
    if (masterEnabled) {
        // Mix the three channels together. We already mixed the busses together
        // with the channel gains and overall master gain.
        SampleUtil::copy3WithGain(m_pMaster,
                m_pOutputBusBuffers[EngineChannel::LEFT], 1.0,
                m_pOutputBusBuffers[EngineChannel::CENTER], 1.0,
                m_pOutputBusBuffers[EngineChannel::RIGHT], 1.0,
                iBufferSize);
        if (m_pMasterTalkoverMix->toBool())
          SampleUtil::addWithGain(m_pMaster,m_pTalkover,1.0,iBufferSize);
        // Process master channel effects
        if (m_pEngineEffectsManager)
        {
            auto masterFeatures = GroupFeatureState{};
            // Well, this is delayed by one buffer (it's dependent on the
            // output). Oh well.
            if (m_pVumeter)  m_pVumeter->collectFeatures(&masterFeatures);
            m_pEngineEffectsManager->process(m_masterHandle.handle(), m_pMaster,iBufferSize, iSampleRate,masterFeatures);
        }
        // Apply master gain after effects.
        auto master_gain = m_pMasterGain->get();
        SampleUtil::applyRampingGain(m_pMaster, m_masterGainOld,master_gain, iBufferSize);
        m_masterGainOld = master_gain;
        // Balance values
        auto balright = CSAMPLE{1};
        auto balleft  = CSAMPLE{1};
        auto bal      = static_cast<CSAMPLE>(m_pBalance->get());
        if (bal > 0.)      balleft  -= bal;
        else if (bal < 0.) balright += bal;
        // Perform balancing on main out
        SampleUtil::applyAlternatingGain(m_pMaster, balleft, balright, iBufferSize);
        // Submit master samples to the side chain to do shoutcasting, recording,
        // etc. (cpu intensive non-realtime tasks)
        auto pSidechain = m_pMaster;
        if (m_pSideChain)
        {
            if (m_pMasterTalkoverMix->toBool())
            {
                // Add Talkover to Sidechain output, re-use the talkover buffer
                SampleUtil::addWithGain(m_pTalkover,m_pMaster, 1.0,iBufferSize);
                pSidechain = m_pTalkover;
            }
            m_pSideChain->writeSamples(pSidechain, iBufferSize);
        }
        // Update VU meter (it does not return anything). Needs to be here so that
        // master balance and talkover is reflected in the VU meter.
        if (m_pVumeter)  m_pVumeter->process(pSidechain, iBufferSize);
        // Add master to headphone with appropriate gain
        if (headphoneEnabled)
        {
            SampleUtil::addWithRampingGain(m_pHead, m_pMaster,m_headphoneMasterGainOld,cmaster_gain, iBufferSize);
            m_headphoneMasterGainOld = cmaster_gain;
        }
    }
    if (headphoneEnabled) {
        // Process headphone channel effects
        if (m_pEngineEffectsManager)
        {
            auto headphoneFeatures = GroupFeatureState{};
            m_pEngineEffectsManager->process(m_headphoneHandle.handle(),m_pHead,iBufferSize, iSampleRate,headphoneFeatures);
        }
        // Head volume
        auto headphoneGain = m_pHeadGain->get();
        SampleUtil::applyRampingGain(m_pHead, m_headphoneGainOld,headphoneGain, iBufferSize);
        m_headphoneGainOld = headphoneGain;
    }
    if (masterEnabled && headphoneEnabled)
    {
        // If Head Split is enabled, replace the left channel of the pfl buffer
        // with a mono mix of the headphone buffer, and the right channel of the pfl
        // buffer with a mono mix of the master output buffer.
        if (m_pHeadSplitEnabled->get())
        {
            // note: NOT VECTORIZED because of in place copy
            for (auto i = decltype(iBufferSize){0},e = iBufferSize/2; i < e; i ++) {
                m_pHead[i * 2 + 0] = (m_pHead  [i * 2 + 0] + m_pHead  [i * 2 + 1]) * CSAMPLE{0.5};
                m_pHead[i * 2 + 1] = (m_pMaster[i * 2 + 0] + m_pMaster[i * 2 + 1]) * CSAMPLE{0.5};
            }
        }
    }
    if (m_pMasterMonoMixdown->get()) {SampleUtil::mixStereoToMono(m_pMaster, m_pMaster, iBufferSize);}
    if (masterEnabled) {m_pMasterDelay->process(m_pMaster, iBufferSize);}
    else {SampleUtil::clear(m_pMaster, iBufferSize);}
    if (headphoneEnabled) {m_pHeadDelay->process(m_pHead, iBufferSize);}
    // We're close to the end of the callback. Wake up the engine worker
    // scheduler so that it runs the workers.
    m_pWorkerScheduler->runWorkers();
}
void EngineMaster::addChannel(EngineChannel* pChannel) {
    auto group = pChannel->getGroup();
    auto handle = m_channelHandleFactory.getOrCreateHandle(group);
    auto pChannelInfo = new ChannelInfo(handle, m_channels.size(),pChannel, this);
    m_channels.append(pChannelInfo);
    auto gainCacheDefault = GainCache{0, false};
    m_channelHeadphoneGainCache.append(gainCacheDefault);
    m_channelTalkoverGainCache.append(gainCacheDefault);
    m_channelMasterGainCache.append(gainCacheDefault);
    // Pre-allocate scratch buffers to avoid memory allocation in the
    // callback. QVarLengthArray does nothing if reserve is called with a size
    // smaller than its pre-allocation.
    m_activeChannels.reserve(m_channels.size());
    m_activeBusChannels[EngineChannel::LEFT].reserve(m_channels.size());
    m_activeBusChannels[EngineChannel::CENTER].reserve(m_channels.size());
    m_activeBusChannels[EngineChannel::RIGHT].reserve(m_channels.size());
    m_activeHeadphoneChannels.reserve(m_channels.size());
    m_activeTalkoverChannels.reserve(m_channels.size());
    if(auto pBuffer = pChannelInfo->m_pChannel->getEngineBuffer()) pBuffer->bindWorkers(m_pWorkerScheduler);
}
EngineChannel* EngineMaster::getChannel(QString group)
{
    for ( auto pChannelInfo : m_channels )
    {
      if ( pChannelInfo && pChannelInfo->m_pChannel->getGroup() == group ) return pChannelInfo->m_pChannel;
    }
    return nullptr;
}
const CSAMPLE* EngineMaster::getDeckBuffer(unsigned int i) const
{
    return getChannelBuffer(PlayerManager::groupForDeck(i));
}
const CSAMPLE* EngineMaster::getOutputBusBuffer(unsigned int i) const
{
    if (i <= EngineChannel::RIGHT) return m_pOutputBusBuffers[i]; else return nullptr;
}
const CSAMPLE* EngineMaster::getChannelBuffer(QString group) const
{
    for ( auto pChannelInfo : m_channels )
    {
      if ( pChannelInfo->m_pChannel->getGroup() == group ) return &pChannelInfo->m_pBuffer[0];
    }
    return nullptr;
}
const CSAMPLE* EngineMaster::buffer(AudioOutput output) const {
    switch (output.getType())
    {
    case AudioOutput::MASTER:
        return getMasterBuffer();
        break;
    case AudioOutput::HEADPHONES:
        return getHeadphoneBuffer();
        break;
    case AudioOutput::BUS:
        return getOutputBusBuffer(output.getIndex());
        break;
    case AudioOutput::DECK:
        return getDeckBuffer(output.getIndex());
        break;
    default:
        return nullptr;
    }
}
void EngineMaster::onOutputConnected(AudioOutput output) {
    switch (output.getType()) {
        case AudioOutput::MASTER:
            // overwrite config option if a master output is configured
            m_pMasterEnabled->set(1.0);
            break;
        case AudioOutput::HEADPHONES:
            m_pHeadphoneEnabled->set(1.0);
            break;
        case AudioOutput::BUS:
            m_bBusOutputConnected[output.getIndex()] = true;
            break;
        case AudioOutput::DECK:
            // We don't track enabled decks.
            break;
        default:
            break;
    }
}

void EngineMaster::onOutputDisconnected(AudioOutput output) {
    switch (output.getType()) {
        case AudioOutput::MASTER:
            // not used, because we need the master buffer for headphone mix
            // and recording/broadcasting as well
            break;
        case AudioOutput::HEADPHONES:
            m_pHeadphoneEnabled->set(1.0);
            break;
        case AudioOutput::BUS:
            m_bBusOutputConnected[output.getIndex()] = false;
            break;
        case AudioOutput::DECK:
            // We don't track enabled decks.
            break;
        default:
            break;
    }
}
