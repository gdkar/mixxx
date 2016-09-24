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

#ifndef SOUNDDEVICE_H
#define SOUNDDEVICE_H

#include <QString>
#include <QList>

#include "soundio/soundmanager.h"
#include "util/result.h"

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

const QString kNetworkDeviceInternalName = "Network stream";

class SoundDevice : public QObject {
    Q_OBJECT
  public:
    SoundDevice(UserSettingsPointer config, SoundManager* sm);
    virtual ~SoundDevice();

    QString getInternalName() const;
    QString getDisplayName() const;
    QString getHostAPI() const;

    void setSampleRate(double sampleRate);
    void setFramesPerBuffer(uint32_t framesPerBuffer);
    virtual Result open(bool isClkRefDevice, int syncBuffers) = 0;
    virtual bool isOpen() const = 0;
    virtual Result close() = 0;
    virtual void readProcess() = 0;
    virtual void writeProcess() = 0;
    virtual QString getError() const = 0;
    virtual uint32_t getDefaultSampleRate() const = 0;
    int getNumOutputChannels() const;
    int getNumInputChannels() const;
    SoundDeviceError addOutput(const AudioOutputBuffer& out);
    SoundDeviceError addInput(const AudioInputBuffer& in);
    QList<AudioInputBuffer> inputs() const;
    QList<AudioOutputBuffer> outputs() const;

    void clearOutputs();
    void clearInputs();
    bool operator==(const SoundDevice &other) const;
    bool operator==(const QString &other) const;

  protected:
    void composeOutputBuffer(CSAMPLE* outputBuffer,
                            uint32_t iFramesPerBuffer,
                            uint32_t readOffset,
                            uint32_t iFrameSize);

    void composeInputBuffer(const CSAMPLE* inputBuffer,
                            uint32_t framesToPush,
                            uint32_t framesWriteOffset,
                            uint32_t iFrameSize);

    void clearInputBuffer(uint32_t framesToPush,
                          uint32_t framesWriteOffset);

    UserSettingsPointer m_pConfig;
    // Pointer to the SoundManager object which we'll request audio from.
    SoundManager* m_pSoundManager;
    // The name of the soundcard, used internally (may include the device ID)
    QString m_strInternalName;
    // The name of the soundcard, as displayed to the user
    QString m_strDisplayName;
    // The number of output channels that the soundcard has
    int m_iNumOutputChannels;
    // The number of input channels that the soundcard has
    int m_iNumInputChannels;
    // The current samplerate for the sound device.
    double m_dSampleRate;
    // The name of the audio API used by this device.
    QString m_hostAPI;
    uint32_t m_framesPerBuffer;
    QList<AudioOutputBuffer> m_audioOutputs;
    QList<AudioInputBuffer> m_audioInputs;
};

#endif
