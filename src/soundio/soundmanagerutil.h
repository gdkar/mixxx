/**
 * @file soundmanagerutil.h
 * @author Bill Good <bkgood at gmail dot com>
 * @date 20100611
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SOUNDMANAGERUTIL_U
#define SOUNDMANAGERUTIL_U

#include <QString>
#include <QMutex>
#include <QDomElement>
#include <QList>

#include "util/types.h"
#include "util/fifo.h"

/**
 * @class ChannelGroup
 * @brief Describes a group of channels, typically a pair for stereo sound in
 *        Mixxx.
 */
class ChannelGroup {
public:
    constexpr ChannelGroup()
    : m_channelBase(0)
    , m_channels(0)
    {}
    constexpr ChannelGroup(uint8_t channelBase, uint8_t channels)
    : m_channelBase(channelBase)
    , m_channels(channels) {}
    constexpr ChannelGroup(const ChannelGroup&) noexcept = default;
    constexpr ChannelGroup(ChannelGroup&&) noexcept = default;
    ChannelGroup&operator=(const ChannelGroup&) noexcept = default;
    ChannelGroup&operator=(ChannelGroup&&) noexcept = default;

    constexpr uint8_t  getChannelBase() const { return m_channelBase;}
    constexpr uint8_t  getChannelCount() const{ return m_channels;}
    constexpr bool operator==(const ChannelGroup &other) const
    {
        return m_channelBase == other.m_channelBase && m_channels == other.m_channels;
    }
    bool clashesWith(const ChannelGroup &other) const;
    constexpr uint32_t  getHash() const
    {
        return (uint32_t(m_channels)<<8)|m_channelBase;
    }
private:
    uint8_t m_channelBase; // base (first) channel used on device
    uint8_t m_channels; // number of channels used (s/b 2 in most cases)
};

/**
 * @class AudioPath
 * @brief Describes a path for audio to take.
 * @note This needs a new name, the current one sucks. If you find one,
 *       feel free to rename as necessary.
 */
class AudioPath {
    Q_GADGET
public:
    // XXX if you add a new type here, be sure to add it to the various
    // methods including getStringFromType, isIndexed, getTypeFromInt,
    // channelsNeededForType (if necessary), the subclasses' getSupportedTypes
    // (if necessary), etc. -- bkgood
    enum AudioPathType {
        MASTER,
        HEADPHONES,
        BUS,
        DECK,
        VINYLCONTROL,
        MICROPHONE,
        AUXILIARY,
        SIDECHAIN,
        INVALID, // if this isn't last bad things will happen -bkgood
    };
    Q_ENUM(AudioPathType);
    constexpr AudioPath(uint8_t  channelBase, uint8_t channels)
    :  m_type(INVALID)
    ,  m_channelGroup(channelBase,channels)
    ,  m_index(0)
    {}
    constexpr AudioPathType getType() const { return m_type;}
    constexpr ChannelGroup getChannelGroup() const { return m_channelGroup;}
    constexpr uint8_t getIndex() const { return m_index;}
    constexpr bool operator==(const AudioPath &other) const
    {
        return m_type == other.m_type && m_index == other.m_index;
    }
    constexpr uint32_t getHash() const
    {
        return (uint32_t(m_type) << 8)|m_index;
    }
    bool channelsClash(const AudioPath &other) const;
    QString getString() const;
    static QString getStringFromType(AudioPathType type);
    static QString getTrStringFromType(AudioPathType type, uint8_t index);
    static AudioPathType getTypeFromString(QString string);
    static constexpr bool isIndexed(AudioPathType type)
    {
        switch(type) {
            case BUS: case DECK: case VINYLCONTROL: case AUXILIARY: case MICROPHONE:
                return true;
            default:
                return false;
        }
    }
    static AudioPathType getTypeFromInt(int typeInt);

    // Returns the minimum number of channels needed on a sound device for an
    // AudioPathType.
    static uint8_t minChannelsForType(AudioPathType type);

    // Returns the maximum number of channels needed on a sound device for an
    // AudioPathType.
    static uint8_t maxChannelsForType(AudioPathType type);

protected:
    virtual void setType(AudioPathType type) = 0;
    AudioPathType m_type{INVALID};
    ChannelGroup  m_channelGroup{};
    uint8_t       m_index{};
};

/**
 * @class AudioOutput
 * @extends AudioPath
 * @brief A source of audio in Mixxx that is to be output to a group of
 *        channels on an audio interface.
 */
class AudioOutput : public AudioPath {
  public:
    AudioOutput(AudioPathType type, uint8_t channelBase,
                uint8_t channels,
                uint8_t index = 0);
    virtual ~AudioOutput();
    QDomElement toXML(QDomElement *element) const;
    static AudioOutput fromXML(const QDomElement &xml);
    static QList<AudioPathType> getSupportedTypes();
    bool isHidden();
  protected:
    void setType(AudioPathType type);
};

// This class is required to add the buffer, without changing the hash used as ID
class AudioOutputBuffer : public AudioOutput {
  public:
    AudioOutputBuffer(const AudioOutput& out, const CSAMPLE* pBuffer)
           : AudioOutput(out),
             m_pBuffer(pBuffer) {

    };
    inline const CSAMPLE* getBuffer() const { return m_pBuffer; }
  private:
    const CSAMPLE* m_pBuffer;
};

/**
 * @class AudioInput
 * @extends AudioPath
 * @brief A source of audio at a group of channels on an audio interface
 *        that is be processed in Mixxx.
 */
class AudioInput : public AudioPath {
  public:
    AudioInput(AudioPathType type = INVALID, uint8_t channelBase = 0,
               uint8_t channels = 0, uint8_t index = 0);
    virtual ~AudioInput();
    QDomElement toXML(QDomElement *element) const;
    static AudioInput fromXML(const QDomElement &xml);
    static QList<AudioPathType> getSupportedTypes();
  protected:
    void setType(AudioPathType type);
};

// This class is required to add the buffer, without changing the hash used as
// ID
class AudioInputBuffer : public AudioInput {
  public:
    AudioInputBuffer(const AudioInput& id, CSAMPLE* pBuffer)
            : AudioInput(id),
              m_pBuffer(pBuffer)
    { }
    CSAMPLE* getBuffer() const { return m_pBuffer; }
  private:
    CSAMPLE* m_pBuffer;
};


class AudioOrigin {
public:
    virtual const CSAMPLE* buffer(AudioOutput output) const = 0;

    // This is called by SoundManager whenever an output is connected for this
    // source. When this is called it is guaranteed that no callback is
    // active.
    virtual void onOutputConnected(AudioOutput output) { Q_UNUSED(output); };

    // This is called by SoundManager whenever an output is disconnected for
    // this source. When this is called it is guaranteed that no callback is
    // active.
    virtual void onOutputDisconnected(AudioOutput output) { Q_UNUSED(output); };
};

class AudioDestination {
public:
    // This is called by SoundManager whenever there are new samples from the
    // configured input to be processed. This is run in the clock reference
    // callback thread
    virtual void receiveBuffer(AudioInput input, const CSAMPLE* pBuffer,
                               uint32_t iNumFrames) = 0;

    // This is called by SoundManager whenever an input is configured for this
    // destination. When this is called it is guaranteed that no callback is
    // active.
    virtual void onInputConfigured(AudioInput input) { Q_UNUSED(input); };

    // This is called by SoundManager whenever an input is unconfigured for this
    // destination. When this is called it is guaranteed that no callback is
    // active.
    virtual void onInputUnconfigured(AudioInput input) { Q_UNUSED(input); };
};

typedef AudioPath::AudioPathType AudioPathType;

// globals for QHash
uint32_t qHash(const ChannelGroup &group);
uint32_t qHash(const AudioOutput &output);
uint32_t qHash(const AudioInput &input);

#endif
