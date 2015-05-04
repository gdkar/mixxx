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
#include <QObject>
#include <QTypeInfo>
#include <QMetaType>
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
    ChannelGroup();
    ChannelGroup(unsigned char channelBase, unsigned char channels);
    unsigned char getChannelBase() const;
    unsigned char getChannelCount() const;
    virtual void          setBuffer(CSAMPLE*b){m_buffer=b;}
    virtual CSAMPLE      *buffer()const{return m_buffer;}
    virtual unsigned char getType() const{return m_type;}
    virtual void          setType(unsigned char t){m_type=t;}
    virtual unsigned char getIndex() const{return m_index;}
    virtual void          setIndex(quint8 i){m_index=i;}
    bool operator==(const ChannelGroup &other) const;
    bool clashesWith(const ChannelGroup &other) const;
    virtual unsigned int getHash() const;
private:
    quint64         m_bitmap;
    CSAMPLE        *m_buffer;
    union{
      struct{
    quint32         m_serial;
    quint8          m_type;
    quint8          m_index;
    quint8          m_base;
    quint8          m_span;
      };
      quint64       m_top;
    };
    static QAtomicInteger<quint32>  s_serial;
//    unsigned char m_channelBase; // base (first) channel used on device
//    unsigned char m_channels; // number of channels used (s/b 2 in most cases)
};

/**
 * @class AudioPath
 * @brief Describes a path for audio to take.
 * @note This needs a new name, the current one sucks. If you find one,
 *       feel free to rename as necessary.
 */
class AudioPath {
public:
    // XXX if you add a new type here, be sure to add it to the various
    // methods including getStringFromType, isIndexed, getTypeFromInt,
    // channelsNeededForType (if necessary), the subclasses' getSupportedTypes
    // (if necessary), etc. -- bkgood
    enum AudioPathType {
        INVALID     = -1,
        MASTER      =  1,
        HEADPHONES  =  2,
        BUS         =  3,
        DECK        =  4,
        VINYLCONTROL=  5,
        MICROPHONE  =  6,
        AUXILIARY   =  7,
        NB_TYPES    =  8, // if this isn't last bad things will happen -bkgood
    };
    AudioPath(unsigned char channelBase, unsigned char channels);
    virtual AudioPathType getType() const;
    virtual void          setType(AudioPathType t){m_channelGroup.setType(static_cast<unsigned char>(t));}
    virtual void  setBuffer(CSAMPLE * b){m_channelGroup.setBuffer(b);}
    virtual ChannelGroup getChannelGroup() const {return m_channelGroup;};
    virtual unsigned char getIndex() const {return m_channelGroup.getIndex();}
    virtual void setIndex(quint8 i) {m_channelGroup.setIndex(i) ;}
    virtual const CSAMPLE *buffer() const{return m_channelGroup.buffer();}
    bool operator==(const AudioPath &other) const;
    virtual unsigned int getHash() const;
    bool channelsClash(const AudioPath &other) const;
    QString getString() const;
    static QString getStringFromType(AudioPathType type);
    static QString getTrStringFromType(AudioPathType type, unsigned char index);
    static AudioPathType getTypeFromString(QString string);
    static bool isIndexed(AudioPathType type);
    static AudioPathType getTypeFromInt(int typeInt);
    // Returns the minimum number of channels needed on a sound device for an
    // AudioPathType.
    static unsigned char minChannelsForType(AudioPathType type);
    // Returns the maximum number of channels needed on a sound device for an
    // AudioPathType.
    static unsigned char maxChannelsForType(AudioPathType type);
protected:
    ChannelGroup m_channelGroup;
};

/**
 * @class AudioOutput
 * @extends AudioPath
 * @brief A source of audio in Mixxx that is to be output to a group of
 *        channels on an audio interface.
 */
class AudioOutput : public AudioPath {
  public:
    AudioOutput(AudioPathType type = INVALID, 
                unsigned char channelBase = 0,
                unsigned char channels = 0,
                unsigned char index = 0);
    virtual ~AudioOutput();
    virtual void setBuffer(CSAMPLE* b){AudioPath::setBuffer(b);}
    virtual const CSAMPLE* buffer()const{return m_channelGroup.buffer();}
    QDomElement toXML(QDomElement *element) const;
    static AudioOutput fromXML(const QDomElement &xml);
    static QList<AudioPathType> getSupportedTypes();
protected:
    void setType(AudioPathType type);
};


/**
 * @class AudioInput
 * @extends AudioPath
 * @brief A source of audio at a group of channels on an audio interface
 *        that is be processed in Mixxx.
 */
class AudioInput : public AudioPath {
  public:
    AudioInput(AudioPathType type = INVALID, unsigned char channelBase = 0,
               unsigned char channels = 0, unsigned char index = 0);
    virtual ~AudioInput();
    QDomElement toXML(QDomElement *element) const;
    static AudioInput fromXML(const QDomElement &xml);
    static QList<AudioPathType> getSupportedTypes();
    virtual void setBuffer(CSAMPLE* b){AudioPath::setBuffer(b);}
    virtual CSAMPLE* buffer()const{return m_channelGroup.buffer();}
  protected:
    void setType(AudioPathType type);
};

class AudioSource {
public:
    virtual CSAMPLE* buffer(AudioOutput output) const = 0;
    // This is called by SoundManager whenever an output is connected for this
    // source. When this is called it is guaranteed that no callback is
    // active.
    virtual void onOutputConnected(AudioOutput output) { Q_UNUSED(output); };
    // This is called by SoundManager whenever an output is disconnected for
    // this source. When this is called it is guaranteed that no callback is
    // active.
    virtual void onOutputDisconnected(AudioOutput output) { Q_UNUSED(output); };
};

class AudioSink {
public:
    // This is called by SoundManager whenever there are new samples from the
    // configured input to be processed. This is run in the clock reference
    // callback thread
    virtual void receiveBuffer(AudioInput input, const CSAMPLE* pBuffer,unsigned int iNumFrames) = 0;
    // This is called by SoundManager whenever an input is configured for this
    // destination. When this is called it is guaranteed that no callback is
    // active.
    virtual void onInputConfigured(AudioInput input) { Q_UNUSED(input); };
    // This is called by SoundManager whenever an input is unconfigured for this
    // destination. When this is called it is guaranteed that no callback is
    // active.
    virtual void onInputUnconfigured(AudioInput input) { Q_UNUSED(input); };
};
Q_DECLARE_TYPEINFO(AudioPath,Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(AudioInput);
Q_DECLARE_TYPEINFO(AudioInput,Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(AudioOutput);
Q_DECLARE_TYPEINFO(AudioOutput,Q_PRIMITIVE_TYPE);

typedef AudioPath::AudioPathType AudioPathType;

// globals for QHash
unsigned int qHash(const ChannelGroup &group);
unsigned int qHash(const AudioOutput &output);
unsigned int qHash(const AudioInput &input);

#endif
