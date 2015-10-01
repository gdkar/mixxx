/**
 * @file soundmanager.cpp
 * @author Albert Santoni <gamegod at users dot sf dot net>
 * @author Bill Good <bkgood at gmail dot com>
 * @date 20070815
 */

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QtDebug>
#include <cstring> // for memcpy and strcmp

#include <QLibrary>
#include <portaudio.h>
#include "soundmanager.h"
#include "sounddevice.h"
#include "sounddeviceportaudio.h"
#include "engine/enginemaster.h"
#include "engine/enginebuffer.h"
#include "soundmanagerutil.h"
#include "controlobject.h"
#include "vinylcontrol/defs_vinylcontrol.h"
#include "sampleutil.h"
#include "util/cmdlineargs.h"
typedef PaError (*SetJackClientName)(const char *name);
SoundManager::SoundManager(ConfigObject<ConfigValue> *pConfig, EngineMaster *pMaster)
        : m_pMaster(pMaster),
          m_pConfig(pConfig),
          m_paInitialized(false),
          m_jackSampleRate(-1),
          m_pErrorDevice(nullptr) {
    qDebug() << "PortAudio version:" << Pa_GetVersion() << "text:" << Pa_GetVersionText();
    // TODO(xxx) some of these ControlObject are not needed by soundmanager, or are unused here.
    // It is possible to take them out?
    m_pControlObjectSoundStatusCO = new ControlObject(ConfigKey("[SoundManager]", "status"),this);
    m_pControlObjectSoundStatusCO->set(SOUNDMANAGER_DISCONNECTED);
    m_pControlObjectVinylControlGainCO = new ControlObject(ConfigKey(VINYL_PREF_KEY, "gain"),this);
    //Hack because PortAudio samplerate enumeration is slow as hell on Linux (ALSA dmix sucks, so we can't blame PortAudio)
    m_samplerates.push_back(44100);
    m_samplerates.push_back(48000);
    m_samplerates.push_back(96000);
    queryDevices(); // initializes PortAudio so SMConfig:loadDefaults can do
                    // its thing if it needs to
    if (!m_config.readFromDisk()) { m_config.loadDefaults(this, SoundManagerConfig::ALL); }
    checkConfig();
    m_config.writeToDisk(); // in case anything changed by applying defaults
}
SoundManager::~SoundManager() {
    //Clean up devices.
    clearDeviceList();
    if (m_paInitialized.exchange(false))Pa_Terminate();
}
QList<SoundDevice*> SoundManager::getDeviceList(QString filterAPI, bool bOutputDevices, bool bInputDevices) {
    //qDebug() << "SoundManager::getDeviceList";
    if (filterAPI == "None") { return QList<SoundDevice*>{}; }
    // Create a list of sound devices filtered to match given API and
    // input/output.
    QList<SoundDevice*> filteredDeviceList;
    for(auto device: m_devices) {
        // Skip devices that don't match the API, don't have input channels when
        // we want input devices, or don't have output channels when we want
        // output devices.
        if (device->getHostAPI() != filterAPI ||
                (bOutputDevices && device->getNumOutputChannels() <= 0) ||
                (bInputDevices && device->getNumInputChannels() <= 0)) {
            continue;
        }
        filteredDeviceList.push_back(device);
    }
    return filteredDeviceList;
}
QList<QString> SoundManager::getHostAPIList() const {
    QList<QString> apiList;
    for (PaHostApiIndex i = 0; i < Pa_GetHostApiCount(); i++) {
        const PaHostApiInfo* api = Pa_GetHostApiInfo(i);
        if (api && QString(api->name) != "skeleton implementation") { apiList.push_back(api->name); }
    }
    return apiList;
}
void SoundManager::closeDevices() {
    //qDebug() << "SoundManager::closeDevices()";
    for ( auto & dev:m_devices ) dev->close();
    m_pErrorDevice = nullptr;
    // TODO(rryan): Should we do this before SoundDevice::close()? No! Because
    // then the callback may be running when we call
    // onInputDisconnected/onOutputDisconnected.
    for(auto  pDevice: m_devices) {
        for (auto in: pDevice->inputs()) {
            // Need to tell all registered AudioDestinations for this AudioInput
            // that the input was disconnected.
            for (auto it = m_registeredDestinations.constFind(in);
                 it != m_registeredDestinations.cend() && it.key() == in; ++it) {
                it.value()->onInputUnconfigured(in);
            }
        }
        for(auto out: pDevice->outputs()) {
            // Need to tell all registered AudioSources for this AudioOutput
            // that the output was disconnected.
            for (auto it = m_registeredSources.constFind(out);
                 it != m_registeredSources.cend() && it.key() == out; ++it) {
                it.value()->onOutputDisconnected(out);
            }
        }
    }
    while (!m_inputBuffers.isEmpty()) { if(auto  pBuffer = m_inputBuffers.takeLast()) SampleUtil::free(pBuffer); }
    // Indicate to the rest of Mixxx that sound is disconnected.
    m_pControlObjectSoundStatusCO->set(SOUNDMANAGER_DISCONNECTED);
}
void SoundManager::clearDeviceList() {
    //qDebug() << "SoundManager::clearDeviceList()";
    // Close the devices first.
    closeDevices();
    // Empty out the list of devices we currently have.
    while (!m_devices.empty()) { delete m_devices.takeLast(); }
    if (m_paInitialized) {Pa_Terminate();m_paInitialized = false;}
}
QList<unsigned int> SoundManager::getSampleRates(QString api) const {
    if (api == MIXXX_PORTAUDIO_JACK_STRING) {
        // queryDevices must have been called for this to work, but the
        // ctor calls it -bkgood
        QList<unsigned int> samplerates;
        samplerates.append(m_jackSampleRate);
        return samplerates;
    }
    return m_samplerates;
}
QList<unsigned int> SoundManager::getSampleRates() const { return getSampleRates(""); }
void SoundManager::queryDevices() {
    //qDebug() << "SoundManager::queryDevices()";
    clearDeviceList();
    PaError err = paNoError;
    if (!m_paInitialized.exchange(true)) 
    {
      if((err = Pa_Initialize())!=paNoError)
      {
          qDebug() << "Error:" << Pa_GetErrorText(err);
          m_paInitialized = false;
          return;
      }
    }
    auto iNumDevices = Pa_GetDeviceCount();
    if (iNumDevices < 0) {
        qDebug() << "ERROR: Pa_CountDevices returned" << iNumDevices;
        return;
    }
    for (auto  i = 0; i < iNumDevices; i++) {
        auto deviceInfo = Pa_GetDeviceInfo(i);
        if (!deviceInfo) continue;
        /* deviceInfo fields for quick reference:
            int     structVersion
            const char *    name
            PaHostApiIndex  hostApi
            int     maxInputChannels
            int     maxOutputChannels
            PaTime  defaultLowInputLatency
            PaTime  defaultLowOutputLatency
            PaTime  defaultHighInputLatency
            PaTime  defaultHighOutputLatency
            double  defaultSampleRate
         */
        auto currentDevice = new SoundDevicePortAudio(m_pConfig, this, deviceInfo, i);
        m_devices.push_back(currentDevice);
        if (!strcmp(Pa_GetHostApiInfo(deviceInfo->hostApi)->name, MIXXX_PORTAUDIO_JACK_STRING))
            m_jackSampleRate = deviceInfo->defaultSampleRate;
    }
    // now tell the prefs that we updated the device list -- bkgood
    emit(devicesUpdated());
}
bool SoundManager::setupDevices() {
    // NOTE(rryan): Big warning: This function is concurrent with calls to
    // pushBuffer and onDeviceOutputCallback until closeDevices() below.
    qDebug() << "SoundManager::setupDevices()";
    m_pControlObjectSoundStatusCO->set(SOUNDMANAGER_CONNECTING);
    auto err = true;
    // NOTE(rryan): Do not clear m_pClkRefDevice here. If we didn't touch the
    // SoundDevice that is the clock reference, then it is safe to leave it as
    // it was. Clearing it causes the engine to stop being processed which
    // results in a stuttering noise (sometimes a loud buzz noise at low
    // latencies) when changing devices.
    //m_pClkRefDevice = NULL;
    m_pErrorDevice = nullptr;
    auto devicesAttempted = 0;
    auto devicesOpened = 0;
    auto outputDevicesOpened = 0;
    auto inputDevicesOpened = 0;
    // filter out any devices in the config we don't actually have
    m_config.filterOutputs(this);
    m_config.filterInputs(this);
    // Close open devices. After this call we will not get any more
    // onDeviceOutputCallback() or pushBuffer() calls because all the
    // SoundDevices are closed. closeDevices() blocks and can take a while.
    closeDevices();
    // NOTE(rryan): Documenting for future people touching this class. If you
    // would like to remove the fact that we close all the devices first and
    // then re-open them, I'm with you! The problem is that SoundDevicePortAudio
    // and SoundManager are not thread safe and the way that mutual exclusion
    // between the Qt main thread and the PortAudio callback thread is acheived
    // is that we shut off the PortAudio callbacks for all devices by closing
    // every device first. We then update all the SoundDevice settings
    // (configured AudioInputs/AudioOutputs) and then we re-open them.
    //
    // If you want to solve this issue, you should start by separating the PA
    // callback from the logic in SoundDevicePortAudio. They should communicate
    // via message passing over a request/response FIFO.

    // Instead of clearing m_pClkRefDevice and then assigning it directly,
    // compute the new one then atomically hand off below.
    SoundDevice* pNewMasterClockRef = nullptr;
    // pair is isInput, isOutput
    auto toOpen = QHash<SoundDevice*, QPair<bool, bool> >{};
    for(auto device: m_devices) {
        bool isInput = false;
        bool isOutput = false;
        device->clearInputs();
        device->clearOutputs();
        m_pErrorDevice = device;
        for(auto in: m_config.getInputs().values(device->getInternalName())) {
            isInput = true;
            // TODO(bkgood) look into allocating this with the frames per
            // buffer value from SMConfig
            auto aib = AudioInputBuffer {in, SampleUtil::alloc(MAX_BUFFER_LEN)};
            if (device->addInput(aib) != SOUNDDEVICE_ERROR_OK) {
                delete [] aib.getBuffer();
                closeDevices();
                return false;
            }
            m_inputBuffers.append(aib.getBuffer());
            // Check if any AudioDestination is registered for this AudioInput
            // and call the onInputConnected method.
            for(auto it:m_registeredDestinations.values(in))
            {
              it->onInputConfigured(in);
            }
        }
        for(auto out: m_config.getOutputs().values(device->getInternalName())) {
            isOutput = true;
            // following keeps us from asking for a channel buffer EngineMaster
            // doesn't have -- bkgood
            const auto pBuffer = m_registeredSources.value(out)->buffer(out);
            if (pBuffer == nullptr) {
                qDebug() << "AudioSource returned null for" << out.getString();
                continue;
            }
            auto aob = AudioOutputBuffer{out, pBuffer};
            if(device->addOutput(aob) != SOUNDDEVICE_ERROR_OK)
            {
              closeDevices();
              return false;
            }
            if (out.getType() == AudioOutput::MASTER) 
              pNewMasterClockRef = device;
            else if ((out.getType() == AudioOutput::DECK || out.getType() == AudioOutput::BUS) && !pNewMasterClockRef)
              pNewMasterClockRef = device;
            // Check if any AudioSource is registered for this AudioOutput and
            // call the onOutputConnected method.
            for(auto source : m_registeredSources.values(out))
              source->onOutputConnected(out);
        }
        if (isInput || isOutput) {
            device->setSampleRate(m_config.getSampleRate());
            device->setFramesPerBuffer(m_config.getFramesPerBuffer());
            toOpen[device] = QPair<bool, bool>(isInput, isOutput);
        }
    }
    for(auto device: toOpen.keys()) {
        auto  mode = toOpen[device];
        bool isInput = mode.first;
        bool isOutput = mode.second;
        ++devicesAttempted;
        m_pErrorDevice = device;
        // If we have not yet set a clock source then we use the first
        if (!pNewMasterClockRef) {
            pNewMasterClockRef = device;
            qWarning() << "Output sound device clock reference not set! Using" << device->getDisplayName();
        }
        auto syncBuffers = m_config.getSyncBuffers();
        // If we are in safe mode and using experimental polling support, use
        // the default of 2 sync buffers instead.
        if (CmdlineArgs::Instance().getSafeMode() && syncBuffers == 0)
          syncBuffers = 2;
        if(!(err = device->open(pNewMasterClockRef == device, syncBuffers)))
        {
          closeDevices();
          return false;
        }
        else {
            ++devicesOpened;
            outputDevicesOpened += static_cast<int>(isOutput);
            inputDevicesOpened  += static_cast<int>(isInput );
        }
    }
    if (pNewMasterClockRef)
        qDebug() << "Using" << pNewMasterClockRef->getDisplayName() << "as output sound device clock reference";
    else 
      qDebug() << "No output devices opened, no clock reference device set";
    qDebug() << outputDevicesOpened << "output sound devices opened";
    qDebug() << inputDevicesOpened << "input  sound devices opened";
    m_pControlObjectSoundStatusCO->set(outputDevicesOpened > 0 ? SOUNDMANAGER_CONNECTED : SOUNDMANAGER_DISCONNECTED);
    // returns OK if we were able to open all the devices the user wanted
    if (devicesAttempted == devicesOpened) {
        emit(devicesSetup());
        return true;
    }
    m_pErrorDevice = nullptr;
    return false;
}
SoundDevice* SoundManager::getErrorDevice() const { return m_pErrorDevice; }
SoundManagerConfig SoundManager::getConfig() const { return m_config; }
bool SoundManager::setConfig(SoundManagerConfig config) {
    auto err = true;
    m_config = config;
    checkConfig();

    // certain parts of mixxx rely on this being here, for the time being, just
    // letting those be -- bkgood
    // Do this first so vinyl control gets the right samplerate -- Owen W.
    m_pConfig->set(ConfigKey("[Soundcard]","Samplerate"), ConfigValue(m_config.getSampleRate()));
    err = setupDevices();
    if (err == true) { m_config.writeToDisk(); }
    return err;
}
void SoundManager::checkConfig() {
    if (!m_config.checkAPI(*this)) {
        m_config.setAPI(SoundManagerConfig::kDefaultAPI);
        m_config.loadDefaults(this, SoundManagerConfig::API | SoundManagerConfig::DEVICES);
    }
    if (!m_config.checkSampleRate(*this)) {
        m_config.setSampleRate(SoundManagerConfig::kFallbackSampleRate);
        m_config.loadDefaults(this, SoundManagerConfig::OTHER);
    }
    // Even if we have a two-deck skin, if someone has configured a deck > 2
    // then the configuration needs to know about that extra deck.
    m_config.setCorrectDeckCount(getConfiguredDeckCount());
    // latency checks itself for validity on SMConfig::setLatency()
}
void SoundManager::onDeviceOutputCallback(const unsigned int iFramesPerBuffer) {
    // Produce a block of samples for output. EngineMaster expects stereo
    // samples so multiply iFramesPerBuffer by 2.
    m_pMaster->process(iFramesPerBuffer*2);
}
void SoundManager::pushInputBuffers(const QList<AudioInputBuffer>& inputs, const unsigned int iFramesPerBuffer) {
   for ( const auto &in : inputs ){
        auto  pInputBuffer = in.getBuffer();
        for ( auto it: m_registeredDestinations.values(in))
        {
          it->receiveBuffer(in,pInputBuffer,iFramesPerBuffer);
        }
    }
}
void SoundManager::writeProcess() 
{ 
  for ( auto device : m_devices ) { if ( device ) device->writeProcess();} 
}
void SoundManager::readProcess() 
{
  for ( auto device : m_devices ) {if ( device ) device->readProcess();} 
}
void SoundManager::registerOutput(AudioOutput output, AudioSource *src) {
    if (m_registeredSources.contains(output)) { qDebug() << "WARNING: AudioOutput already registered!"; }
    m_registeredSources.insert(output, src);
    emit(outputRegistered(output, src));
}
void SoundManager::registerInput(AudioInput input, AudioDestination *dest) {
    if (m_registeredDestinations.contains(input)) { qDebug() << "WARNING: AudioInput already registered!"; }
    m_registeredDestinations.insertMulti(input, dest);
    emit(inputRegistered(input, dest));
}
QList<AudioOutput> SoundManager::registeredOutputs() const { return m_registeredSources.keys(); }
QList<AudioInput> SoundManager::registeredInputs() const { return m_registeredDestinations.keys(); }
void SoundManager::setConfiguredDeckCount(int count) {
    m_config.setDeckCount(count);
    checkConfig();
    m_config.writeToDisk();
}
int SoundManager::getConfiguredDeckCount() const 
{ 
  return m_config.getDeckCount(); 
}
