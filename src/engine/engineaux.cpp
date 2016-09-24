// engineaux.cpp
// created 4/8/2011 by Bill Good (bkgood@gmail.com)
// shameless stolen from enginemicrophone.cpp (from RJ)

#include "engine/engineaux.h"

#include <QtDebug>

#include "control/control.h"
#include "preferences/usersettings.h"
#include "control/controlaudiotaperpot.h"
#include "effects/effectsmanager.h"
#include "engine/effects/engineeffectsmanager.h"
#include "util/sample.h"

EngineAux::EngineAux(QObject *p, const ChannelHandleAndGroup& handle_group, EffectsManager* pEffectsManager)
        : EngineChannel(p, handle_group, EngineChannel::CENTER, pEffectsManager),
          m_pPregain(new ControlAudioTaperPot(ConfigKey(getGroup(), "pregain"),this, -12, 12, 0.5)),
          m_sampleBuffer(NULL)
{
    // Make input_configured read-only.
/*    m_pInputConfigured->connectValueChangeRequest(
        this, SLOT(slotInputConfiguredChangeRequest(double)),
        Qt::DirectConnection);*/
    ControlDoublePrivate::insertAlias(ConfigKey(getGroup(), "enabled"),
                                      ConfigKey(getGroup(), "input_configured"));

    // by default Aux is enabled on the master and disabled on PFL. User
    // can over-ride by setting the "pfl" or "master" controls.
    setMaster(true);
}
EngineAux::~EngineAux()
{
    delete m_pPregain;
    delete m_pSampleRate;
}
bool EngineAux::isActive()
{
    auto enabled = m_pInputConfigured->toBool();
    if (enabled && m_sampleBuffer.load()) {
        m_wasActive = true;
    } else if (m_wasActive) {
        m_pVUMeter->reset();
        m_wasActive = false;
    }
    return m_wasActive;
}

void EngineAux::onInputConfigured(AudioInput input) {
    if (input.getType() != AudioPath::AUXILIARY) {
        // This is an error!
        qDebug() << "WARNING: EngineAux connected to AudioInput for a non-auxiliary type!";
        return;
    }
    m_sampleBuffer = nullptr;
    m_pInputConfigured->setAndConfirm(1.0);
}

void EngineAux::onInputUnconfigured(AudioInput input) {
    if (input.getType() != AudioPath::AUXILIARY) {
        // This is an error!
        qDebug() << "WARNING: EngineAux connected to AudioInput for a non-auxiliary type!";
        return;
    }
    m_sampleBuffer = NULL;
    m_pInputConfigured->setAndConfirm(0.0);
}

void EngineAux::receiveBuffer(AudioInput input, const CSAMPLE* pBuffer,unsigned int nFrames)
                              {
    Q_UNUSED(input);
    Q_UNUSED(nFrames);
    m_sampleBuffer = pBuffer;
}

void EngineAux::process(CSAMPLE* pOut, int iBufferSize)
{
    auto sampleBuffer = m_sampleBuffer.load(); // save pointer on stack
    auto pregain =  m_pPregain->get();
    if (sampleBuffer) {
        SampleUtil::copyWithGain(pOut, sampleBuffer, pregain, iBufferSize);
        m_sampleBuffer = NULL;
    } else {
        SampleUtil::clear(pOut, iBufferSize);
    }
    if (m_pEngineEffectsManager) {
        GroupFeatureState features;
        // This is out of date by a callback but some effects will want the RMS
        // volume.
        m_pVUMeter->collectFeatures(&features);
        // Process effects enabled for this channel
        m_pEngineEffectsManager->process(getHandle(), pOut, iBufferSize,m_pSampleRate->get(), features);
    }
    // Update VU meter
    m_pVUMeter->process(pOut, iBufferSize);
}
