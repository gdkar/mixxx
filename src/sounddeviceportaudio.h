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

_Pragma("once")
#include <portaudio.h>
#include <QString>
#include <atomic>
#include <memory>
#include "sounddevice.h"
#define CPU_USAGE_UPDATE_RATE 30 // in 1/s, fits to display frame rate
#define CPU_OVERLOAD_DURATION 500 // in ms

class SoundManager;

/** Dynamically resolved function which allows us to enable a realtime-priority callback
    thread from ALSA/PortAudio. This must be dynamically resolved because PortAudio can't
    tell us if ALSA is compiled into it or not. */
typedef int (*EnableAlsaRT)(PaStream* s, int enable);

class ControlObject;

class SoundDevicePortAudio : public SoundDevice {
  Q_OBJECT
  public:
    SoundDevicePortAudio(ConfigObject<ConfigValue> *config,
                         SoundManager *sm, const PaDeviceInfo *deviceInfo,
                         unsigned int devIndex,QObject*);
    virtual ~SoundDevicePortAudio() ;
    virtual bool open(bool isClkRefDevice, int syncBuffers, int fragments);
    virtual bool close();
    virtual void readProcess();
    virtual int readAvailable();
    virtual void writeProcess();
    virtual int writeAvailable();
    virtual QString getError() const;
    // This callback function gets called everytime the sound device runs out of
    // samples (ie. when it needs more sound to play)
    int callbackProcess(const unsigned int framesPerBuffer,
                        CSAMPLE *output, const CSAMPLE* in,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags);
    // Same as above but with drift correction
    int callbackProcessDrift(const unsigned int framesPerBuffer,
                        CSAMPLE *output, const CSAMPLE* in,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags);
    // The same as above but drives the MixxEngine
    int callbackProcessClkRef(const unsigned int framesPerBuffer,
                        CSAMPLE *output, const CSAMPLE* in,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags);
    virtual unsigned int getDefaultSampleRate() const {
        return m_deviceInfo ? static_cast<unsigned int>(m_deviceInfo->defaultSampleRate) : 44100u;
    }
  private:
    // PortAudio stream for this device.
    std::atomic<PaStream*> m_pStream{nullptr};
    // PortAudio device index for this device.
    PaDeviceIndex m_devId = 0;
    // Struct containing information about this device. Don't free() it, it
    // belongs to PortAudio.
    const PaDeviceInfo* m_deviceInfo = nullptr;
    // Description of the output stream going to the soundcard.
    PaStreamParameters m_outputParams {0,0,0,0,nullptr };
    // Description of the input stream coming from the soundcard.
    PaStreamParameters m_inputParams  {0,0,0,0,nullptr };
    FIFO<CSAMPLE>* m_outputFifo = nullptr;
    FIFO<CSAMPLE>* m_inputFifo  = nullptr;
    bool m_outputDrift = false;
    bool m_inputDrift  = false;
    // A string describing the last PortAudio error to occur.
    QString m_lastError;
    // Whether we have set the thread priority to realtime or not.
    bool m_bSetThreadPriority = false;
    std::unique_ptr<ControlObject> m_pMasterAudioLatencyOverloadCount;
    std::unique_ptr<ControlObject> m_pMasterAudioLatencyUsage        ;
    std::unique_ptr<ControlObject> m_pMasterAudioLatencyOverload     ;
    int m_underflowUpdateCount = 0;
    std::atomic<int> m_underflowCount {0};
    std::atomic<int> m_overflowCount  {0};
    static std::atomic<int> m_underflowHappend;
    qint64 m_nsInAudioCb = 0;
    int m_framesSinceAudioLatencyUsageUpdate = 0;
    int m_syncBuffers = 2;
};
