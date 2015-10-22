/***************************************************************************
                          sounddevice.cpp
                             -------------------
    begin                : Sun Aug 12, 2007, past my bedtime
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
#include <QString>
#include <QList>
#include <memory>
#include <atomic>
#include "soundmanager.h"
#include "util/defs.h"
//Forward declarations
class SoundDevice;
class SoundManager;
class AudioOutput;
class AudioInput;
enum SoundDeviceError {
    SOUNDDEVICE_ERROR_OK = OK,
    SOUNDDEVICE_ERROR_DUPLICATE_OUTPUT_CHANNEL,
    SOUNDDEVICE_ERROR_EXCESSIVE_OUTPUT_CHANNEL,
    SOUNDDEVICE_ERROR_EXCESSIVE_INPUT_CHANNEL,
};
class SoundDevice {
  public:
    SoundDevice(ConfigObject<ConfigValue> *config, SoundManager* sm);
    virtual ~SoundDevice();
    QString getInternalName() const;
    QString getDisplayName() const;
    QString getHostAPI() const;
    void setHostAPI(QString api);
    void setSampleRate(double sampleRate);
    void setFramesPerBuffer(size_t framesPerBuffer);
    virtual bool open(bool isClkRefDevice, int syncBuffers) = 0;
    virtual bool close() = 0;
    virtual void readProcess(size_t ) = 0;
    virtual void writeProcess(size_t ) = 0;
    virtual QString getError() const = 0;
    virtual double getDefaultSampleRate() const = 0;
    int getNumOutputChannels() const;
    int getNumInputChannels() const;
    SoundDeviceError addOutput(const AudioOutputBuffer& out);
    SoundDeviceError addInput(const AudioInputBuffer& in);
    QList<AudioInputBuffer>  inputs() const;
    QList<AudioOutputBuffer> outputs() const;
    void clearOutputs();
    void clearInputs();
    bool operator==(const SoundDevice &other) const;
    bool operator==(QString other) const;
  protected:
    void composeOutputBuffer(CSAMPLE* outputBuffer,
                             size_t iFramesPerBuffer,
                             off_t  readOffset,
                             size_t iFrameSize);
    void composeInputBuffer(const CSAMPLE* inputBuffer,
                            size_t  framesToPush,
                            off_t   framesWriteOffset,
                            size_t  iFrameSize);
    void clearInputBuffer(size_t framesToPush, off_t framesWriteOffset);
    ConfigObject<ConfigValue> *m_pConfig = nullptr;
    // Pointer to the SoundManager object which we'll request audio from.
    SoundManager* m_pSoundManager        = nullptr;
    // The name of the soundcard, used internally (may include the device ID)
    QString m_strInternalName;
    // The name of the soundcard, as displayed to the user
    QString m_strDisplayName;
    // The number of output channels that the soundcard has
    int m_iNumOutputChannels = 0;
    // The number of input channels that the soundcard has
    int m_iNumInputChannels  = 0;
    // The current samplerate for the sound device.
    double m_dSampleRate     = 0.0;
    // The name of the audio API used by this device.
    QString m_hostAPI;
    size_t m_framesPerBuffer = 0;
    QList<AudioOutputBuffer> m_audioOutputs;
    QList<AudioInputBuffer> m_audioInputs;
};
