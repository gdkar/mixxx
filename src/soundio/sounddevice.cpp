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

#include "soundio/sounddevice.h"

#include <QtDebug>
#include <cstring> // for memcpy and strcmp

#include "soundio/soundmanager.h"
#include "soundio/soundmanagerutil.h"
#include "util/debug.h"
#include "util/defs.h"
#include "util/sample.h"

SoundDevice::SoundDevice(UserSettingsPointer config, SoundManager* sm)
        : QObject(sm),
          m_pConfig(config),
          m_pSoundManager(sm),
          m_strInternalName("Unknown Soundcard"),
          m_strDisplayName("Unknown Soundcard"),
          m_iNumOutputChannels(2),
          m_iNumInputChannels(2),
          m_dSampleRate(44100.0),
          m_hostAPI("Unknown API"),
          m_framesPerBuffer(0) {
}

SoundDevice::~SoundDevice() { }

int SoundDevice::getNumInputChannels() const {
    return m_iNumInputChannels;
}

int SoundDevice::getNumOutputChannels() const {
    return m_iNumOutputChannels;
}

void SoundDevice::setSampleRate(double sampleRate) {
    if (sampleRate <= 0.0) {
        // this is the default value used elsewhere in this file
        sampleRate = 44100.0;
    }
    m_dSampleRate = sampleRate;
}

void SoundDevice::setFramesPerBuffer(uint32_t framesPerBuffer) {
    if (framesPerBuffer * 2 > MAX_BUFFER_LEN) {
        // framesPerBuffer * 2 because a frame will generally end up
        // being 2 samples and MAX_BUFFER_LEN is a number of samples
        // this isn't checked elsewhere, so...
        reportFatalErrorAndQuit("framesPerBuffer too big in "
                                "SoundDevice::setFramesPerBuffer(uint)");
    }
    m_framesPerBuffer = framesPerBuffer;
}

SoundDevice::SoundDeviceError SoundDevice::addOutput(const AudioOutputBuffer &out) {
    // Check if the output channels are already used
    foreach (AudioOutputBuffer myOut, m_audioOutputs) {
        if (out.channelsClash(myOut)) {
            return SoundDeviceError::DUPLICATE_OUTPUT_CHANNEL;
        }
    }
    if (out.getChannelGroup().getChannelBase()
            + out.getChannelGroup().getChannelCount() > getNumOutputChannels()) {
        return SoundDeviceError::EXCESSIVE_OUTPUT_CHANNEL;
    }
    m_audioOutputs.append(out);
    return SoundDeviceError::OK;
}

void SoundDevice::clearOutputs() {
    m_audioOutputs.clear();
}

SoundDevice::SoundDeviceError SoundDevice::addInput(const AudioInputBuffer &in) {
    // DON'T check if the input channels are already used, there's no reason
    // we can't send the same inputted samples to different places in mixxx.
    // -- bkgood 20101108
    if (in.getChannelGroup().getChannelBase()
            + in.getChannelGroup().getChannelCount() > getNumInputChannels()) {
        return SoundDeviceError::EXCESSIVE_INPUT_CHANNEL;
    }
    m_audioInputs.append(in);
    return SoundDeviceError::OK;
}

void SoundDevice::clearInputs() {
    m_audioInputs.clear();
}

bool SoundDevice::operator==(const SoundDevice &other) const {
    return this->getInternalName() == other.getInternalName();
}

bool SoundDevice::operator==(const QString &other) const {
    return getInternalName() == other;
}

void SoundDevice::composeOutputBuffer(CSAMPLE* outputBuffer,
                                      uint32_t framesToCompose,
                                      uint32_t framesReadOffset,
                                      uint32_t iFrameSize) {
    //qDebug() << "SoundDevice::composeOutputBuffer()"
    //         << device->getInternalName()
    //         << framesToCompose << iFrameSize;

    // Interlace Audio data onto portaudio buffer.  We iterate through the
    // source list to find out what goes in the buffer data is interlaced in
    // the order of the list

    if (iFrameSize == 2 && m_audioOutputs.size() == 1 &&
            m_audioOutputs.at(0).getChannelGroup().getChannelCount() == 2) {
        // Special case for one stereo device only
        auto &&out = m_audioOutputs.at(0);
        auto pAudioOutputBuffer = out.getBuffer(); // Always Stereo
        pAudioOutputBuffer = &pAudioOutputBuffer[framesReadOffset*2];
        SampleUtil::copyClampBuffer(outputBuffer, pAudioOutputBuffer,
               framesToCompose * 2);
    } else {
        // Reset sample for each open channel
        SampleUtil::clear(outputBuffer, framesToCompose * iFrameSize);

        for (auto i = m_audioOutputs.begin(), e = m_audioOutputs.end(); i != e; ++i) {
            auto & out = *i;

            auto outChans = out.getChannelGroup();
            auto iChannelCount = outChans.getChannelCount();
            auto iChannelBase = outChans.getChannelBase();

            auto pAudioOutputBuffer = out.getBuffer();
            // advanced to offset; pAudioOutputBuffer is always stereo
            pAudioOutputBuffer = &pAudioOutputBuffer[framesReadOffset*2];
            if (iChannelCount == 1) {
                // All AudioOutputs are stereo as of Mixxx 1.12.0. If we have a mono
                // output then we need to downsample.
                for (auto iFrameNo = 0u; iFrameNo < framesToCompose; ++iFrameNo) {
                    // iFrameBase is the "base sample" in a frame (ie. the first
                    // sample in a frame)
                    auto iFrameBase = iFrameNo * iFrameSize;
                    outputBuffer[iFrameBase + iChannelBase] = SampleUtil::clampSample(
                            (pAudioOutputBuffer[iFrameNo * 2] +
                                    pAudioOutputBuffer[iFrameNo * 2 + 1]) / 2.0f);
                }
            } else {
                for (auto iFrameNo = 0u; iFrameNo < framesToCompose; ++iFrameNo) {
                    // iFrameBase is the "base sample" in a frame (ie. the first
                    // sample in a frame)
                    auto iFrameBase = iFrameNo * iFrameSize;
                    auto iLocalFrameBase = iFrameNo * iChannelCount;

                    // this will make sure a sample from each channel is copied
                    for (auto iChannel = 0; iChannel < iChannelCount; ++iChannel) {
                        outputBuffer[iFrameBase + iChannelBase + iChannel] =
                                SampleUtil::clampSample(
                                        pAudioOutputBuffer[iLocalFrameBase + iChannel]);

                        // Input audio pass-through (useful for debugging)
                        //if (in)
                        //    output[iFrameBase + src.channelBase + iChannel] =
                        //    in[iFrameBase + src.channelBase + iChannel];
                    }
                }
            }
        }
    }
}

void SoundDevice::composeInputBuffer(const CSAMPLE* inputBuffer,
                                     uint32_t framesToPush,
                                     uint32_t framesWriteOffset,
                                     uint32_t iFrameSize) {
    //qDebug() << "SoundManager::pushBuffer"
    //         << framesToPush << framesWriteOffset << iFrameSize;
    // This function is called a *lot* and is a big source of CPU usage.
    // It needs to be very fast.

    // If the framesize is only 2, then we only have one pair of input channels
    //  That means we don't have to do any deinterlacing, and we can pass
    //  the audio on to its intended destination.
    if (iFrameSize == 1 && m_audioInputs.size() == 1 &&
            m_audioInputs.at(0).getChannelGroup().getChannelCount() == 1) {
        // One mono device only
        auto && in = m_audioInputs.at(0);
        auto pInputBuffer = in.getBuffer(); // Always Stereo
        pInputBuffer = &pInputBuffer[framesWriteOffset * 2];
        for (auto iFrameNo = 0u; iFrameNo < framesToPush; ++iFrameNo) {
            pInputBuffer[iFrameNo * 2] = inputBuffer[iFrameNo];
            pInputBuffer[iFrameNo * 2 + 1] = inputBuffer[iFrameNo];
        }
    } else if (iFrameSize == 2 && m_audioInputs.size() == 1 &&
            m_audioInputs.at(0).getChannelGroup().getChannelCount() == 2) {
        // One stereo device only
        auto && in = m_audioInputs.at(0);
        auto pInputBuffer = in.getBuffer(); // Always Stereo
        pInputBuffer = &pInputBuffer[framesWriteOffset * 2];
        SampleUtil::copy(pInputBuffer, inputBuffer, framesToPush * 2);
    } else {
        // Non Stereo input (iFrameSize != 2)
        // Do crazy deinterleaving of the audio into the correct m_inputBuffers.

        for (auto i = m_audioInputs.begin(),
                     e = m_audioInputs.end(); i != e; ++i) {
            auto && in = *i;
            auto chanGroup = in.getChannelGroup();
            auto iChannelCount = chanGroup.getChannelCount();
            auto iChannelBase = chanGroup.getChannelBase();
            auto pInputBuffer = in.getBuffer();
            pInputBuffer = &pInputBuffer[framesWriteOffset * 2];

            for (auto iFrameNo = 0u; iFrameNo < framesToPush; ++iFrameNo) {
                // iFrameBase is the "base sample" in a frame (ie. the first
                // sample in a frame)
                auto iFrameBase = iFrameNo * iFrameSize;
                auto iLocalFrameBase = iFrameNo * 2;

                if (iChannelCount == 1) {
                    pInputBuffer[iLocalFrameBase] =
                            inputBuffer[iFrameBase + iChannelBase];
                    pInputBuffer[iLocalFrameBase + 1] =
                            inputBuffer[iFrameBase + iChannelBase];
                } else if (iChannelCount > 1) {
                    pInputBuffer[iLocalFrameBase] =
                            inputBuffer[iFrameBase + iChannelBase];
                    pInputBuffer[iLocalFrameBase + 1] =
                            inputBuffer[iFrameBase + iChannelBase + 1];
                }
            }
        }
    }
}
void SoundDevice::clearInputBuffer(uint32_t framesToPush,
                                   uint32_t framesWriteOffset) {

    for (auto i = m_audioInputs.begin(), e = m_audioInputs.end(); i != e; ++i) {
        auto && in = *i;
        auto pInputBuffer = in.getBuffer();  // Always stereo
        SampleUtil::clear(&pInputBuffer[framesWriteOffset * 2], framesToPush * 2);
    }
}
QList<AudioInputBuffer> SoundDevice::inputs() const
{
    return m_audioInputs;
}
QList<AudioOutputBuffer> SoundDevice::outputs() const
{
    return m_audioOutputs;
}
QString SoundDevice::getInternalName() const
{
    return m_strInternalName;
}
QString SoundDevice::getDisplayName() const
{
    return m_strDisplayName;
}
QString SoundDevice::getHostAPI() const
{
    return m_hostAPI;
}

