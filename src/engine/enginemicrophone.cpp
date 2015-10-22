// enginemicrophone.cpp
// created 3/16/2011 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>

#include "engine/enginemicrophone.h"

#include "configobject.h"
#include "sampleutil.h"
#include "effects/effectsmanager.h"
#include "engine/effects/engineeffectsmanager.h"
#include "controlaudiotaperpot.h"
#include "controlobject.h"

EngineMicrophone::EngineMicrophone(const ChannelHandleAndGroup& handle_group,
                                   EffectsManager* pEffectsManager,
                                   QObject *pParent)
        : EngineChannel(handle_group, EngineChannel::CENTER,pParent),
          m_pEngineEffectsManager(pEffectsManager ? pEffectsManager->getEngineEffectsManager() : nullptr),
          m_vuMeter(getGroup()),
          m_pEnabled(new ControlObject(ConfigKey(getGroup(), "enabled"),this)),
          m_pPregain(new ControlAudioTaperPot(ConfigKey(getGroup(), "pregain"), -12, 12, 0.5)),
          m_sampleBuffer(nullptr),
          m_wasActive(false)
{
    if (pEffectsManager ) pEffectsManager->registerChannel(handle_group);
    setMaster(false); // Use "talkover" button to enable microphones
    m_pSampleRate = new ControlObject(ConfigKey("Master", "samplerate"),this);
}
EngineMicrophone::~EngineMicrophone()
{
    qDebug() << "~EngineMicrophone()";
    delete m_pEnabled;
    delete m_pPregain;
}
bool EngineMicrophone::isActive()
{
    auto enabled = m_pEnabled->get() > 0.0;
    if (enabled && m_sampleBuffer) 
        m_wasActive = true;
    else if (m_wasActive)
    {
        m_vuMeter.reset();
        m_wasActive = false;
    }
    return m_wasActive;
}
void EngineMicrophone::onInputConfigured(AudioInput input)
{
    if (input.getType() != AudioPath::MICROPHONE) {
        // This is an error!
        qWarning() << "EngineMicrophone connected to AudioInput for a non-Microphone type!";
        return;
    }
    m_sampleBuffer = nullptr;
    m_pEnabled->set(1.0);
}
void EngineMicrophone::onInputUnconfigured(AudioInput input)
{
    if (input.getType() != AudioPath::MICROPHONE) {
        // This is an error!
        qWarning() << "EngineMicrophone connected to AudioInput for a non-Microphone type!";
        return;
    }
    m_sampleBuffer = nullptr;
    m_pEnabled->set(0.0);
}
void EngineMicrophone::receiveBuffer(AudioInput /*input*/, const CSAMPLE* pBuffer,unsigned int /*nFrames*/)
{
    m_sampleBuffer = pBuffer;
}
void EngineMicrophone::process(CSAMPLE* pOut, const int iBufferSize)
{
    // If configured read into the output buffer.
    // Otherwise, skip the appropriate number of samples to throw them away.
    auto sampleBuffer = m_sampleBuffer; // save pointer on stack
    auto pregain =  m_pPregain->get();
    if (sampleBuffer) SampleUtil::copyWithGain(pOut, sampleBuffer, pregain, iBufferSize);
    else SampleUtil::clear(pOut, iBufferSize);
    m_sampleBuffer = nullptr;
    if (m_pEngineEffectsManager)
    {
        // Process effects enabled for this channel
        GroupFeatureState features;
        // This is out of date by a callback but some effects will want the RMS
        // volume.
        m_vuMeter.collectFeatures(&features);
        m_pEngineEffectsManager->process(getHandle(), pOut, iBufferSize,m_pSampleRate->get(), features);
    }
    // Update VU meter
    m_vuMeter.process(pOut, iBufferSize);
}
