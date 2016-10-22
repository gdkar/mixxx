#ifndef MIXXX_AUDIOSIGNAL_H
#define MIXXX_AUDIOSIGNAL_H

#include "util/assert.h"
#include "util/types.h"
#include <QAudioFormat>
#include <QAudioBuffer>
namespace mixxx {

// Common properties of audio signals in Mixxx.
//
// An audio signal describes a stream of samples for multiple channels.
// Internally each sample is represented by a floating-point value.
//
// The properties of an audio signal are immutable and must be constant
// over time. Therefore all functions for modifying individual properties
// are declared as "protected" and are only available from derived classes.
class AudioSignal {
public:
    // Defines the ordering of how samples from multiple channels are
    // stored in contiguous buffers:
    //    - Planar: Channel by channel
    //    - Interleaved: Frame by frame
    // The samples from all channels that are coincident in time are
    // called a "frame" (or more specific "sample frame").
    //
    // Example: 10 stereo samples from left (L) and right (R) channel
    // Planar layout:      LLLLLLLLLLRRRRRRRRRR
    // Interleaved layout: LRLRLRLRLRLRLRLRLRLR
    enum class SampleLayout {
        Planar,
        Interleaved
    };

    static constexpr SINT kChannelCountZero    = 0;
    static constexpr SINT kChannelCountDefault = kChannelCountZero;
    static constexpr SINT kChannelCountMono    = 1;
    static constexpr SINT kChannelCountMin     = kChannelCountMono; // lower bound
    static constexpr SINT kChannelCountStereo  = 2;
    static const SINT kChannelCountMax     = 256; // upper bound (8-bit unsigned integer)

    static constexpr bool isValidChannelCount(SINT channelCount)
    {
        return (kChannelCountMin <= channelCount) && (kChannelCountMax >= channelCount);
    }

    static constexpr SINT kSampleRateZero    = 0;
    static constexpr SINT kSampleRateDefault = kSampleRateZero;
    static constexpr SINT kSampleRateMin     = 8000; // lower bound (= minimum MP3 sampling rate)
    static constexpr SINT kSampleRate32kHz   = 32000;
    static constexpr SINT kSampleRateCD      = 44100;
    static constexpr SINT kSampleRate48kHz   = 48000;
    static constexpr SINT kSampleRate96kHz   = 96000;
    static constexpr SINT kSampleRate192kHz  = 192000;
    static constexpr SINT kSampleRateMax     = kSampleRate192kHz; // upper bound

    static constexpr bool isValidSampleRate(SINT samplingRate) {
        return (kSampleRateMin <= samplingRate) && (kSampleRateMax >= samplingRate);
    }

    explicit AudioSignal(SampleLayout sampleLayout)
        : m_sampleLayout(sampleLayout),
          m_channelCount(kChannelCountDefault),
          m_samplingRate(kSampleRateDefault)
    {
        DEBUG_ASSERT(!hasValidChannelCount());
        DEBUG_ASSERT(!hasValidSampleRate());
        m_qAudioFormat.setChannelCount(m_channelCount);
        m_qAudioFormat.setSampleSize(32);
        m_qAudioFormat.setSampleType(QAudioFormat::Float);
        m_qAudioFormat.setSampleRate(m_samplingRate);
        m_qAudioFormat.setByteOrder(QAudioFormat::LittleEndian);
        m_qAudioFormat.setCodec("audio/pcm");
    }
    AudioSignal(SampleLayout sampleLayout, SINT channelCount, SINT samplingRate)
        : m_sampleLayout(sampleLayout),
          m_channelCount(channelCount),
          m_samplingRate(samplingRate)
    {
        DEBUG_ASSERT(kChannelCountZero <= m_channelCount); // unsigned value
        DEBUG_ASSERT(kSampleRateZero <= m_samplingRate); // unsigned value
        m_qAudioFormat.setChannelCount(m_channelCount);
        m_qAudioFormat.setSampleSize(32);
        m_qAudioFormat.setSampleType(QAudioFormat::Float);
        m_qAudioFormat.setSampleRate(m_samplingRate);
        m_qAudioFormat.setByteOrder(QAudioFormat::LittleEndian);
        m_qAudioFormat.setCodec("audio/pcm");
    }
    virtual ~AudioSignal() = default;
    // Returns the ordering of samples in contiguous buffers
    SampleLayout getSampleLayout() const
    {
        return m_sampleLayout;
    }
    // Returns the number of channels.
    SINT getChannelCount() const
    {
        return m_channelCount;
    }
    bool hasValidChannelCount() const
    {
        return isValidChannelCount(getChannelCount());
    }
    // Returns the sampling rate in Hz. The sampling rate is defined as the
    // number of samples per second for each channel. Please not that this
    // does not equal the total number of samples per second in the stream!
    //
    // NOTE(uklotzde): I consciously avoided the term "sample rate", because
    // that sounds like "number of samples per second" which is wrong for
    // signals with more than a single channel and might be misleading!
    SINT getSampleRate() const
    {
        return m_samplingRate;
    }
    bool hasValidSampleRate() const
    {
        return isValidSampleRate(getSampleRate());
    }

    // Verifies various properties to ensure that the audio data is
    // actually readable. Warning messages are logged for properties
    // with invalid values for diagnostic purposes.
    //
    // Subclasses may override this function for checking additional
    // properties in derived classes. Derived functions should always
    // call the implementation of the super class first:
    //
    // bool DerivedClass::verifyReadable() const {
    //     bool result = BaseClass::validate();
    //     if (my property is invalid) {
    //         qWarning() << ...warning message...
    //         result = false;
    //     }
    //     return result;
    // }
    virtual bool verifyReadable() const;
    // Conversion: #samples / sample offset -> #frames / frame offset
    template<typename T>
    T samples2frames(T samples) const
    {
        DEBUG_ASSERT(hasValidChannelCount());
        DEBUG_ASSERT(0 == (samples % getChannelCount()));
        return samples / getChannelCount();
    }
    // Conversion: #frames / frame offset -> #samples / sample offset
    template<typename T>
    T frames2samples(T frames) const
    {
        DEBUG_ASSERT(hasValidChannelCount());
        return frames * getChannelCount();
    }
    const QAudioFormat &format() const
    {
        return m_qAudioFormat;
    }
protected:
    void setChannelCount(SINT channelCount)
    {
        DEBUG_ASSERT(kChannelCountZero <= m_channelCount); // unsigned value
        m_channelCount = channelCount;
        m_qAudioFormat.setChannelCount(channelCount);
    }
    void resetChannelCount()
    {
        setChannelCount(kChannelCountDefault);
    }
    void setSampleRate(SINT samplingRate)
    {
        DEBUG_ASSERT(kSampleRateZero <= m_samplingRate); // unsigned value
        m_samplingRate = samplingRate;
        m_qAudioFormat.setSampleRate(samplingRate);
    }
    void resetSampleRate()
    {
        setSampleRate(kSampleRateDefault);
    }
private:
    SampleLayout m_sampleLayout;
    SINT m_channelCount;
    SINT m_samplingRate;
    QAudioFormat m_qAudioFormat{};
};
}
#endif // MIXXX_AUDIOSIGNAL_H
