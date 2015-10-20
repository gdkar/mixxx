/***************************************************************************
                          sounddeviceportaudio.cpp
                             -------------------
    begin                : Sun Aug 15, 2007 (Stardate -315378.5417935057)
    copyright            : (C) 2007 Albert Santoni
    email                : gamegod \a\t users.sf.net
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <portaudio.h>
#include <QtDebug>
#include <QThread>

#ifdef __LINUX__
#include <QLibrary>
#endif
#include <memory>
#include <utility>
#include <algorithm>
#include "sounddeviceportaudio.h"

#include "soundmanager.h"
#include "sounddevice.h"
#include "soundmanagerutil.h"
#include "controlobject.h"
#include "visualplayposition.h"
#include "util/timer.h"
#include "util/trace.h"
#include "vinylcontrol/defs_vinylcontrol.h"
#include "sampleutil.h"
#include "util/performancetimer.h"
#include "util/denormalsarezero.h"
namespace /* anonymous */ {
  int paV19Callback(const void *inputBuffer, void *outputBuffer,
                    size_t framesPerBuffer,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *soundDevice) {
      return ((SoundDevicePortAudio*)soundDevice)->callbackProcess((unsigned int)framesPerBuffer,
              (CSAMPLE*)outputBuffer, (const CSAMPLE*)inputBuffer, timeInfo, statusFlags);
  }
  int paV19CallbackDrift(const void *inputBuffer, void *outputBuffer,
                    size_t framesPerBuffer,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *soundDevice) {
      return ((SoundDevicePortAudio*)soundDevice)->callbackProcessDrift((unsigned int)framesPerBuffer,
              (CSAMPLE*)outputBuffer, (const CSAMPLE*)inputBuffer, timeInfo, statusFlags);
  }

  int paV19CallbackClkRef(const void *inputBuffer, void *outputBuffer,
                    size_t framesPerBuffer,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *soundDevice) {
      return ((SoundDevicePortAudio*)soundDevice)->callbackProcessClkRef((unsigned int)framesPerBuffer,
              (CSAMPLE*)outputBuffer, (const CSAMPLE*)inputBuffer, timeInfo, statusFlags);
  }
  const int kDriftReserve = 1; // Buffer for drift correction 1 full, 1 for r/w, 1 empty
  const int kFifoSize = 2 * kDriftReserve + 1; // Buffer for drift correction 1 full, 1 for r/w, 1 empty
}
// static
std::atomic<int> SoundDevicePortAudio::m_underflowHappend{0};
SoundDevicePortAudio::SoundDevicePortAudio(
      ConfigObject<ConfigValue> *config
    , SoundManager *sm
    , const PaDeviceInfo *deviceInfo
    , unsigned int devIndex
    , QObject *p
    )
        : SoundDevice(config, sm,p),
          m_devId(devIndex),
          m_deviceInfo(deviceInfo){
    // Setting parent class members:
    m_hostAPI = Pa_GetHostApiInfo(deviceInfo->hostApi)->name;
    m_dSampleRate = deviceInfo->defaultSampleRate;
    m_strInternalName = QString("%1, %2").arg(QString::number(m_devId), deviceInfo->name);
    m_strDisplayName = QString::fromLocal8Bit(deviceInfo->name);
    m_iNumInputChannels = m_deviceInfo->maxInputChannels;
    m_iNumOutputChannels = m_deviceInfo->maxOutputChannels;

    m_pMasterAudioLatencyOverloadCount = std::make_unique<ControlObject>(ConfigKey("Master", "audio_latency_overload_count"));
    m_pMasterAudioLatencyUsage = std::make_unique<ControlObject>(ConfigKey("Master", "audio_latency_usage"));
    m_pMasterAudioLatencyOverload  = std::make_unique<ControlObject>(ConfigKey("Master", "audio_latency_overload"));
}
SoundDevicePortAudio::~SoundDevicePortAudio()=default;
bool SoundDevicePortAudio::open(bool isClkRefDevice, int syncBuffers,int fragments)
{
    qDebug() << "SoundDevicePortAudio::open()" << getInternalName();
    PaError err;
    if (m_audioOutputs.empty() && m_audioInputs.empty()) {
        m_lastError = QString::fromAscii("No inputs or outputs in SDPA::open() "
            "(THIS IS A BUG, this should be filtered by SM::setupDevices)");
        return false;
    }
    std::memset(&m_outputParams, 0, sizeof(m_outputParams));
    std::memset(&m_inputParams, 0, sizeof(m_inputParams));
    auto pOutputParams = &m_outputParams;
    auto pInputParams  = &m_inputParams;
    // Look at how many audio outputs we have,
    // so we can figure out how many output channels we need to open.
    if (m_audioOutputs.empty()) {
        m_outputParams.channelCount = 0;
        pOutputParams = nullptr;
    }
    else
    {
        for(auto &out: m_audioOutputs)
        {
            auto channelGroup = out.getChannelGroup();
            auto highChannel = channelGroup.getChannelBase() + channelGroup.getChannelCount();
            if (m_outputParams.channelCount <= highChannel) m_outputParams.channelCount = highChannel;
        }
    }
    // Look at how many audio inputs we have,
    // so we can figure out how many input channels we need to open.
    if (m_audioInputs.empty())
    {
        m_inputParams.channelCount = 0;
        pInputParams = nullptr;
    }
    else
    {
        for(auto &in: m_audioInputs)
        {
            auto channelGroup = in.getChannelGroup();
            auto highChannel = channelGroup.getChannelBase() + channelGroup.getChannelCount();
            if (m_inputParams.channelCount <= highChannel) m_inputParams.channelCount = highChannel;
        }
    }
    // Workaround for Bug #900364. The PortAudio ALSA hostapi opens the minimum
    // number of device channels supported by the device regardless of our
    // channel request. It has no way of notifying us when it does this. The
    // typical case this happens is when we are opening a device in mono when it
    // supports a minimum of stereo. To work around this, simply open the device
    // in stereo and only take the first channel.
    // TODO(rryan): Remove once PortAudio has a solution built in (and
    // released).
    if (m_deviceInfo->hostApi == paALSA)
    {
        // Only engage workaround if the device has enough input and output
        // channels.
        if (m_deviceInfo->maxInputChannels >= 2 && m_inputParams.channelCount == 1) m_inputParams.channelCount = 2;
        if (m_deviceInfo->maxOutputChannels >= 2 && m_outputParams.channelCount == 1) m_outputParams.channelCount = 2;
    }
    // Sample rate
    if (m_dSampleRate <= 0) m_dSampleRate = 44100.0;
    // Get latency in milleseconds
    qDebug() << "framesPerBuffer:" << m_framesPerBuffer;
    auto bufferMSec = m_framesPerBuffer / m_dSampleRate * 1000;
    qDebug() << "Requested sample rate: " << m_dSampleRate << "Hz, latency:" << bufferMSec << "ms";
    qDebug() << "Output channels:" << m_outputParams.channelCount << "| Input channels:" << m_inputParams.channelCount;
    // PortAudio's JACK backend also only properly supports
    // paFramesPerBufferUnspecified in non-blocking mode because the latency
    // comes from the JACK daemon. (PA should give an error or something though,
    // but it doesn't.)
    if (m_deviceInfo->hostApi == paJACK) m_framesPerBuffer = paFramesPerBufferUnspecified;
    //Fill out the rest of the info.
    m_outputParams.device = m_devId;
    m_outputParams.sampleFormat = paFloat32;
    m_outputParams.suggestedLatency = bufferMSec / 1000.0;
    m_outputParams.hostApiSpecificStreamInfo = nullptr;

    m_inputParams.device  = m_devId;
    m_inputParams.sampleFormat  = paFloat32;
    m_inputParams.suggestedLatency = bufferMSec / 1000.0;
    m_inputParams.hostApiSpecificStreamInfo = nullptr;
    qDebug() << "Opening stream with id" << m_devId;
    m_syncBuffers = syncBuffers;
    //Create the callback function pointer.
    auto callback = static_cast<PaStreamCallback*>(nullptr);
    if ( fragments < 3 ) fragments = 3;
    if (m_outputParams.channelCount)
    {
        // On chunk for reading one for writing and on for drift correction
        m_outputFifo = new FIFO<CSAMPLE>(m_outputParams.channelCount * m_framesPerBuffer * fragments);
        // Clear first 1.5 chunks on for the required artificial delaly to a allow jitter
        // and a half, because we can't predict which callback fires first.
        m_outputFifo->releaseWriteRegions(m_outputParams.channelCount * m_framesPerBuffer * fragments / 2);
    }
    if (m_inputParams.channelCount)
    {
        m_inputFifo = new FIFO<CSAMPLE>(m_inputParams.channelCount * m_framesPerBuffer * fragments);
        // Clear first two 1.5 chunks (see above)
        m_inputFifo->releaseWriteRegions(m_inputParams.channelCount * m_framesPerBuffer * fragments / 2);
    }
    if (isClkRefDevice)             callback = paV19CallbackClkRef;
    else if (m_syncBuffers == 2)    callback = paV19CallbackDrift;
    else if (m_syncBuffers == 1)    callback = paV19Callback;
    auto pStream = static_cast<PaStream*>(nullptr);
    // Try open device using iChannelMax
    err = Pa_OpenStream(&pStream,
                        pInputParams,
                        pOutputParams,
                        m_dSampleRate,
                        m_framesPerBuffer,
                        paClipOff, // Stream flags
                        callback,
                        static_cast<void*>( this )); // pointer passed to the callback function
    if (err != paNoError)
    {
        qWarning() << "Error opening stream:" << Pa_GetErrorText(err);
        m_lastError = QString::fromUtf8(Pa_GetErrorText(err));
        return false;
    }
    else qDebug() << "Opened PortAudio stream successfully... starting";
#ifdef __LINUX__
    //Attempt to dynamically load and resolve stuff in the PortAudio library
    //in order to enable RT priority with ALSA.
    QLibrary portaudio("libportaudio.so.2");
    if (!portaudio.load()) qWarning() << "Failed to dynamically load PortAudio library";
    else                   qDebug() << "Dynamically loaded PortAudio library";
    auto enableRealtime = reinterpret_cast<EnableAlsaRT>(portaudio.resolve("PaAlsa_EnableRealtimeScheduling"));
    if (enableRealtime) enableRealtime(pStream, 1);
    portaudio.unload();
#endif
    // Start stream
    if ((err = Pa_StartStream(pStream)) != paNoError) 
    {
        qWarning() << "PortAudio: Start stream error:" << Pa_GetErrorText(err);
        m_lastError = QString::fromUtf8(Pa_GetErrorText(err));
        if((err = Pa_CloseStream(pStream))!=paNoError)
          qWarning() << "PortAudio: Close stream error:" << Pa_GetErrorText(err) << getInternalName();
        return false;
    } 
    else qDebug() << "PortAudio: Started stream successfully";
    // Get the actual details of the stream & update Mixxx's data
    auto streamDetails = Pa_GetStreamInfo(pStream);
    m_dSampleRate = streamDetails->sampleRate;
    auto currentLatencyMSec = streamDetails->outputLatency * 1000;
    qDebug() << "   Actual sample rate: " << m_dSampleRate << "Hz, latency:" << currentLatencyMSec << "ms";
    if (isClkRefDevice)
    {
        // Update the samplerate and latency ControlObjects, which allow the
        // waveform view to properly correct for the latency.
        ControlObject::set(ConfigKey("Master", "latency"), currentLatencyMSec);
        ControlObject::set(ConfigKey("Master", "samplerate"), m_dSampleRate);
        ControlObject::set(ConfigKey("Master", "audio_buffer_size"), bufferMSec);
        if (m_pMasterAudioLatencyOverloadCount) m_pMasterAudioLatencyOverloadCount->set(0);
    }
    m_pStream.store(pStream);
    return true;
}
bool SoundDevicePortAudio::close()
{
    //qDebug() << "SoundDevicePortAudio::close()" << getInternalName();
    auto pStream = m_pStream.exchange(nullptr);
    if (pStream)
    {
        // Make sure the stream is not stopped before we try stopping it.
        auto err = Pa_IsStreamStopped(pStream);
        // 1 means the stream is stopped. 0 means active.
        if (err == 1) return true;
        // Real PaErrors are always negative.
        if (err < 0)
        {
            qWarning() << "PortAudio: Stream already stopped:" << Pa_GetErrorText(err) << getInternalName();
            return false;
        }
        //PaError err = Pa_AbortStream(m_pStream); //Trying Pa_AbortStream instead, because StopStream seems to wait
                                                   //until all the buffers have been flushed, which can take a
                                                   //few (annoying) seconds when you're doing soundcard input.
                                                   //(it flushes the input buffer, and then some, or something)
                                                   //BIG FAT WARNING: Pa_AbortStream() will kill threads while they're
                                                   //waiting on a mutex, which will leave the mutex in an screwy
                                                   //state. Don't use it!
        if ((err = Pa_StopStream(pStream)) != paNoError)
        {
            qWarning() << "PortAudio: Stop stream error:" << Pa_GetErrorText(err) << getInternalName();
            return false;
        }
        // Close stream
        if ((err = Pa_CloseStream(pStream)) != paNoError)
        {
            qWarning() << "PortAudio: Close stream error:" << Pa_GetErrorText(err) << getInternalName();
            return false;
        }
        if (m_outputFifo) 
          delete m_outputFifo;
        if (m_inputFifo) 
          delete m_inputFifo;
    }
    m_outputFifo = nullptr;
    m_inputFifo = nullptr;
    m_bSetThreadPriority = false;
    return true;
}
QString SoundDevicePortAudio::getError() const
{
  return m_lastError;
}
void SoundDevicePortAudio::readProcess()
{
    auto  pStream = m_pStream.load();
    if (pStream && m_inputParams.channelCount && m_inputFifo)
    {
        auto inChunkSize = m_framesPerBuffer * m_inputParams.channelCount;
        auto ravail = m_inputFifo->readAvailable();
        auto readCount = inChunkSize;
        if (inChunkSize > ravail )
        {
            readCount = ravail;
            m_underflowHappend = 1;
        }
        if (readCount)
        {
            auto dataPtr1 = static_cast<CSAMPLE*>(nullptr);
            auto size1 = PaUtilRingBuffer<CSAMPLE>::size_type{0};
            auto dataPtr2 = static_cast<CSAMPLE*>(nullptr);
            auto size2 = PaUtilRingBuffer<CSAMPLE>::size_type{0};
            // We use size1 and size2, so we can ignore the return value
            (void)m_inputFifo->aquireReadRegions(readCount, &dataPtr1, &size1, &dataPtr2, &size2);
            // Fetch fresh samples and write to the the output buffer
            composeInputBuffer(dataPtr1,size1 / m_inputParams.channelCount, 0,m_inputParams.channelCount);
            if (size2 > 0)
            {
                composeInputBuffer(dataPtr2,size2 / m_inputParams.channelCount, size1 / m_inputParams.channelCount,m_inputParams.channelCount);
            }
            m_inputFifo->releaseReadRegions(readCount);
        }
        if (readCount < inChunkSize) clearInputBuffer(inChunkSize - readCount, readCount);
        m_pSoundManager->pushInputBuffers(m_audioInputs, m_framesPerBuffer);
    }
}
void SoundDevicePortAudio::writeProcess()
{
    auto pStream = m_pStream.load();
    if (pStream && m_outputParams.channelCount && m_outputFifo)
    {
        auto outChunkSize = m_framesPerBuffer * m_outputParams.channelCount;
        auto wavail= m_outputFifo->writeAvailable();
        auto writeCount = outChunkSize;
        if (outChunkSize > wavail)
        {
            writeCount = wavail;
            m_underflowHappend = 1;
            //qDebug() << "writeProcess():" << (float) writeAvailable / outChunkSize << "Overflow";
        }
        if (writeCount)
        {
            auto dataPtr1 = static_cast<CSAMPLE*>(nullptr);
            auto size1 = PaUtilRingBuffer<CSAMPLE>::size_type{0};
            auto dataPtr2 = static_cast<CSAMPLE*>(nullptr);
            auto size2 = PaUtilRingBuffer<CSAMPLE>::size_type{0};
            // We use size1 and size2, so we can ignore the return value
            (void)m_outputFifo->aquireWriteRegions(writeCount, &dataPtr1, &size1, &dataPtr2, &size2);
            // Fetch fresh samples and write to the the output buffer
            composeOutputBuffer(dataPtr1, size1 / m_outputParams.channelCount, 0, static_cast<unsigned int>(m_outputParams.channelCount));
            if (size2 > 0)
            {
                composeOutputBuffer(dataPtr2, size2 / m_outputParams.channelCount, size1 / m_outputParams.channelCount,
                        static_cast<unsigned int>(m_outputParams.channelCount));
            }
            m_outputFifo->releaseWriteRegions(writeCount);
        }
    }
}
int SoundDevicePortAudio::callbackProcessDrift(const unsigned int framesPerBuffer,
                                          CSAMPLE *out, const CSAMPLE *in,
                                          const PaStreamCallbackTimeInfo *timeInfo,
                                          PaStreamCallbackFlags statusFlags)
{
    Q_UNUSED(timeInfo);
    Trace trace("SoundDevicePortAudio::callbackProcessDrift %1", getInternalName());
    if ( m_outputFifo && m_outputFifo->writeAvailable() >=  m_outputParams.channelCount * m_framesPerBuffer )needProcess (  );
    if (statusFlags & (paOutputUnderflow | paInputOverflow)) m_underflowHappend = 1;
    // Since we are on the non Clock reference device and may have an independent
    // Crystal clock, a drift correction is required
    //
    // There is a delay of up to one latency between composing a chunk in the Clock
    // Reference callback and write it to the device. So we need at lest one buffer.
    // Unfortunately this delay is somehow random, an WILL produce a delay slow
    // shift without we can avoid it. (Thats the price for using a cheap USB soundcard).
    //
    // Additional we need an filled chunk and an empty chunk. These are used when on
    // sound card overtakes the other. This always happens, if they are driven form
    // two crystals. In a test case every 30 s @ 23 ms. After they are consumed,
    // the drift correction takes place and fills or clears the reserve buffers.
    // If this is finished before an other overtake happens, we do not face any
    // dropouts or clicks.
    // So thats why we need a Fifo of 3 chunks.
    //
    // In addition there is a jitter effect. It happens that one callback is delayed,
    // in this case the second one fires two times and then the first one fires two
    // time as well to catch up. This is also fixed by the additional buffers. If this
    // happens just after an regular overtake, we will have clicks again.
    //
    // I the tests it turns out that it only happens in the opposite direction, so
    // 3 chunks are just fine.
    if (m_inputParams.channelCount)
    {
        auto inChunkSize = framesPerBuffer * m_inputParams.channelCount;
        auto ravail= m_inputFifo->readAvailable();
        auto wavail= m_inputFifo->writeAvailable();
        if (ravail< inChunkSize * kDriftReserve)
        {
            // risk of an underflow, duplicate one frame
            m_inputFifo->write(in, inChunkSize);
            if (m_inputDrift)
            {
                // Do not compensate the first delay, because it is likely a jitter
                // corrected in the next cycle
                // Duplicate one frame
                m_inputFifo->write(&in[inChunkSize - m_inputParams.channelCount], m_inputParams.channelCount);
            }
            else m_inputDrift = true;
        }
        else if (ravail== inChunkSize * kDriftReserve)
        {
            // Everything Ok
            m_inputFifo->write(in, inChunkSize);
            m_inputDrift = false;
        }
        else if (wavail>= inChunkSize)
        {
            // Risk of overflow, skip one frame
            if (m_inputDrift) m_inputFifo->write(in, inChunkSize - m_inputParams.channelCount);
            else
            {
                m_inputFifo->write(in, inChunkSize);
                m_inputDrift = true;
            }
        }
        else if (wavail)
        {
            // Fifo Overflow
            m_inputFifo->write(in, wavail);
            m_underflowHappend = 1;
        } else m_underflowHappend = 1;
    }
    if (m_outputParams.channelCount)
    {
        auto outChunkSize = framesPerBuffer * m_outputParams.channelCount;
        auto ravail= m_outputFifo->readAvailable();
        if (ravail> outChunkSize * (kDriftReserve + 1))
        {
            m_outputFifo->read(out, outChunkSize);
            if (m_outputDrift) m_outputFifo->releaseReadRegions(m_outputParams.channelCount);
            else m_outputDrift = true;
        }
        else if (ravail== outChunkSize * (kDriftReserve + 1))
        {
            m_outputFifo->read(out,outChunkSize);
            m_outputDrift = false;
        }
        else if (ravail>= outChunkSize)
        {
            if (m_outputDrift)
            {
                // Risk of underflow, duplicate one frame
                m_outputFifo->read(out, outChunkSize - m_outputParams.channelCount);
                SampleUtil::copy(&out[outChunkSize - m_outputParams.channelCount],
                       &out[outChunkSize - (2 * m_outputParams.channelCount)],
                        m_outputParams.channelCount);
            }
            else
            {
                m_outputFifo->read(out, outChunkSize);
                m_outputDrift = true;
            }
        }
        else if (ravail)
        {
            m_outputFifo->read(out,ravail);
            // underflow
            SampleUtil::clear(&out[ravail],outChunkSize - ravail);
            m_underflowHappend = 1;
        }
        else
        {
            // underflow
            SampleUtil::clear(out, outChunkSize);
            m_underflowHappend = 1;
        }
     }
    return paContinue;
}
int SoundDevicePortAudio::callbackProcess(const unsigned int framesPerBuffer,
                                          CSAMPLE *out, const CSAMPLE *in,
                                          const PaStreamCallbackTimeInfo *timeInfo,
                                          PaStreamCallbackFlags statusFlags)
{
    Q_UNUSED(timeInfo);
    Trace trace("SoundDevicePortAudio::callbackProcess %1", getInternalName());
    if ( m_outputFifo && m_outputFifo->writeAvailable() >=  m_outputParams.channelCount * m_framesPerBuffer )
    {
      emit needProcess (  );
    }
    if (statusFlags & (paOutputUnderflow | paInputOverflow)) m_underflowHappend = 1;
    if (m_inputParams.channelCount)
    {
        auto inChunkSize = framesPerBuffer * m_inputParams.channelCount;
        auto wavail= m_inputFifo->writeAvailable();
        if (wavail>= inChunkSize) m_inputFifo->write(in, inChunkSize - m_inputParams.channelCount);
        else if (wavail)
        {
            // Fifo Overflow
            m_inputFifo->write(in, wavail);
            m_underflowHappend = 1;
            //qDebug() << "callbackProcess write:" << "Overflow";
        }
        else m_underflowHappend = 1;
    }
    if (m_outputParams.channelCount)
    {
        auto outChunkSize = framesPerBuffer * m_outputParams.channelCount;
        auto ravail= m_outputFifo->readAvailable();
        if (ravail>= outChunkSize) m_outputFifo->read(out, outChunkSize);
        else if (ravail)
        {
            m_outputFifo->read(out,ravail);
            // underflow
            SampleUtil::clear(&out[ravail],outChunkSize - ravail);
            m_underflowHappend = 1;
        }
        else
        {
            // underflow
            SampleUtil::clear(out, outChunkSize);
            m_underflowHappend = 1;
        }
     }
    return paContinue;
}
int SoundDevicePortAudio::callbackProcessClkRef(const unsigned int bufferSize,
                                                CSAMPLE *out, const CSAMPLE *in,
                                                const PaStreamCallbackTimeInfo *timeInfo,
                                                PaStreamCallbackFlags statusFlags)
{
    PerformanceTimer timer;
    timer.start();
    Trace trace("SoundDevicePortAudio::callbackProcessClkRef %1", getInternalName());
    //qDebug() << "SoundDevicePortAudio::callbackProcess:" << getInternalName();
    // Turn on TimeCritical priority for the callback thread. If we are running
    // in Linux userland, for example, this will have no effect.
    VisualPlayPosition::setTimeInfo(timeInfo);
    if (statusFlags & (paOutputUnderflow | paInputOverflow)) m_underflowHappend = true;
    if (m_underflowUpdateCount == 0)
    {
        if (m_underflowHappend)
        {
            m_pMasterAudioLatencyOverload->set(1.0);
            m_pMasterAudioLatencyOverloadCount->set(m_pMasterAudioLatencyOverloadCount->get() + 1);
            m_underflowUpdateCount = CPU_OVERLOAD_DURATION * m_dSampleRate / bufferSize / 1000;
            m_underflowHappend = 0; // reseting her is not thread save,
                                    // but that is OK, because we count only
                                    // 1 underflow each 500 ms
        }
        else m_pMasterAudioLatencyOverload->set(0.0);
    }
    else --m_underflowUpdateCount;
    m_framesSinceAudioLatencyUsageUpdate += bufferSize ;
    if (m_framesSinceAudioLatencyUsageUpdate > (m_dSampleRate / CPU_USAGE_UPDATE_RATE))
    {
        auto secInAudioCb = (double)m_nsInAudioCb / 1000000000.0;
        m_pMasterAudioLatencyUsage->set(secInAudioCb / (m_framesSinceAudioLatencyUsageUpdate / m_dSampleRate));
        m_nsInAudioCb = 0;
        m_framesSinceAudioLatencyUsageUpdate = 0;
    }
    if ( m_outputParams.channelCount && m_outputFifo->readAvailable() >= m_framesPerBuffer * m_outputParams.channelCount)
    {
      emit needProcess();
    }
    if (m_inputParams.channelCount)
    {
        auto inChunkSize = bufferSize * m_inputParams.channelCount;
        auto wavail= m_inputFifo->writeAvailable();
        if (wavail>= inChunkSize) m_inputFifo->write(in, inChunkSize - m_inputParams.channelCount);
        else if (wavail)
        {
            // Fifo Overflow
            m_inputFifo->write(in, wavail);
            m_underflowHappend = 1;
            //qDebug() << "callbackProcess write:" << "Overflow";
        }
        else m_underflowHappend = 1;
    }
//    m_pSoundManager->pushInputBuffers(m_audioInputs, m_framesPerBuffer);
//    m_pSoundManager->readProcess();
//    m_pSoundManager->onDeviceOutputCallback(framesPerBuffer);
//    m_pSoundManager->writeProcess();
    if (m_outputParams.channelCount)
    {
        auto outChunkSize = bufferSize * m_outputParams.channelCount;
        auto ravail = m_outputFifo->readAvailable();
        if (ravail >= outChunkSize) m_outputFifo->read(out, outChunkSize);
        else if (ravail )
        {
            m_outputFifo->read(out,ravail);
            // underflow
            SampleUtil::clear(&out[ravail],outChunkSize - ravail);
            m_underflowHappend = 1;
        }
        else
        {
            // underflow
            SampleUtil::clear(out, outChunkSize);
            m_underflowHappend = 1;
        }
     }

    m_nsInAudioCb += timer.elapsed();
    return paContinue;
}
int SoundDevicePortAudio::readAvailable()
{
  if( ! m_inputFifo || !m_inputParams.channelCount ) return std::numeric_limits<int>::max();
  return m_inputFifo->readAvailable() / m_inputParams.channelCount;
}
int SoundDevicePortAudio::writeAvailable()
{
  if( !m_outputFifo || !m_outputParams.channelCount ) return std::numeric_limits<int>::max();
  return m_outputFifo->writeAvailable() / m_outputParams.channelCount;
}
