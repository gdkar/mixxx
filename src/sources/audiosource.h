_Pragma("once")
#include <QSharedPointer>
#include <QUrl>
#include "util/audiosignal.h"
#include "util/assert.h"
#include "util/result.h"
#include "util/samplebuffer.h"

namespace Mixxx {

// forward declaration(s)
class AudioSourceConfig;

// Common interface and base class for audio sources.
//
// Both the number of channels and the sampling rate must
// be constant and are not allowed to change over time.
//
// The length of audio data is measured in frames. A frame
// is a tuple containing samples from each channel that are
// coincident in time. A frame for a mono signal contains a
// single sample. A frame for a stereo signal contains a pair
// of samples, one for the left and right channel respectively.
//
// Samples in a sample buffer are stored as consecutive frames,
// i.e. the samples of the channels are interleaved.
//
// Audio sources are implicitly opened upon creation and
// closed upon destruction.
class AudioSource: public AudioSignal {
  const QUrl m_url;
  public:
    QUrl getUrl() const;
    QString getUrlString() const;
    bool    isLocalFile() const;
    QString getLocalFileName() const;
    static const SampleLayout kSampleLayout = SampleLayout::Interleaved;
    // Returns the total number of sample frames.
    SINT getFrameCount() const;
    bool isEmpty() const;
    // The actual duration in seconds.
    // Well defined only for valid files!
    bool hasDuration() const;
    SINT getDuration() const;
    // The bitrate is measured in kbit/s (kbps).
    static bool isValidBitrate(SINT bitrate);
    bool hasBitrate() const;
    // Setting the bitrate is optional when opening a file.
    // The bitrate is not needed for decoding, it is only used
    // for informational purposes.
    SINT getBitrate() const;
    // Index of the first sample frame.
    static SINT getMinFrameIndex();
    // Index of the sample frame following the last
    // sample frame.
    SINT getMaxFrameIndex() const;
    // The sample frame index is valid in the range
    // [getMinFrameIndex(), getMaxFrameIndex()].
    bool isValidFrameIndex(SINT frameIndex) const;
    // Adjusts the current frame seek index:
    // - Precondition: isValidFrameIndex(frameIndex) == true
    //   - Index of first frame: frameIndex = 0
    //   - Index of last frame: frameIndex = getFrameCount() - 1
    // - The seek position in seconds is frameIndex / getSamplingRate()
    // Returns the actual current frame index which may differ from the
    // requested index if the source does not support accurate seeking.
    virtual SINT seekSampleFrame(SINT frameIndex) = 0;

    // Fills the buffer with samples from each channel starting
    // at the current frame seek position.
    //
    // The implicit  minimum required capacity of the sampleBuffer is
    //     sampleBufferSize = frames2samples(numberOfFrames)
    // Samples in the sampleBuffer are stored as consecutive sample
    // frames with samples from each channel interleaved.
    //
    // Returns the actual number of frames that have been read which
    // might be lower than the requested number of frames when the end
    // of the audio stream has been reached. The current frame seek
    // position is moved forward towards the next unread frame.
    virtual SINT readSampleFrames(
            SINT numberOfFrames,
            CSAMPLE* sampleBuffer) = 0;

    SINT skipSampleFrames(SINT numberOfFrames);
    SINT readSampleFrames(SINT numberOfFrames,SampleBuffer* pSampleBuffer);

    // Specialized function for explicitly reading stereo (= 2 channels)
    // frames from an AudioSource. This is the preferred method in Mixxx
    // to read a stereo signal.
    //
    // If the source provides only a single channel (mono) the samples
    // of that channel will be doubled. If the source provides more
    // than 2 channels only the first 2 channels will be read.
    //
    // Most audio sources in Mixxx implicitly reduce multi-channel output
    // to stereo during decoding. Other audio sources override this method
    // with an optimized version that does not require a second pass through
    // the sample data or that avoids the allocation of a temporary buffer
    // when reducing multi-channel data to stereo.
    //
    // The minimum required capacity of the sampleBuffer is
    //     sampleBufferSize = numberOfFrames * 2
    // In order to avoid the implicit allocation of a temporary buffer
    // when reducing multi-channel to stereo the caller must provide
    // a sample buffer of size
    //     sampleBufferSize = frames2samples(numberOfFrames)
    //
    // Returns the actual number of frames that have been read which
    // might be lower than the requested number of frames when the end
    // of the audio stream has been reached. The current frame seek
    // position is moved forward towards the next unread frame.
    //
    // Derived classes may provide an optimized version that doesn't
    // require any post-processing as done by this default implementation.
    // They may also have reduced space requirements on sampleBuffer,
    // i.e. only the minimum size is required for an in-place
    // transformation without temporary allocations.
    virtual SINT readSampleFramesStereo(
            SINT numberOfFrames,
            CSAMPLE* sampleBuffer,
            SINT sampleBufferSize);

    SINT readSampleFramesStereo(SINT numberOfFrames,SampleBuffer* pSampleBuffer);

    // Utility function to clamp the frame index interval
    // [*pMinFrameIndexOfInterval, *pMaxFrameIndexOfInterval)
    // to valid frame indexes. The lower bound is inclusive and
    // the upper bound is exclusive!
    static void clampFrameInterval(
            SINT* pMinFrameIndexOfInterval,
            SINT* pMaxFrameIndexOfInterval,
            SINT maxFrameIndexOfAudioSource);

  protected:
    explicit AudioSource(const QUrl& url);
    explicit AudioSource(const AudioSource& other) = default;
    explicit AudioSource(AudioSource&& outer) = default;
    inline static bool isValidFrameCount(SINT frameCount) {
        return kFrameCountZero <= frameCount;
    }
    void setFrameCount(SINT frameCount);
    void setBitrate(SINT bitrate);
    SINT getSampleBufferSize(
            SINT numberOfFrames,
            bool readStereoSamples = false) const;
  private:
    friend class AudioSourceConfig;

    static const SINT kFrameCountZero = 0;
    static const SINT kFrameCountDefault = kFrameCountZero;

    // 0-based indexing of sample frames
    static const SINT kFrameIndexMin = 0;

    static const SINT kBitrateZero = 0;
    static const SINT kBitrateDefault = kBitrateZero;

    SINT m_frameCount;

    SINT m_bitrate;
};

// Parameters for configuring audio sources
class AudioSourceConfig : public AudioSignal {
  public:
    AudioSourceConfig()
        : AudioSignal(AudioSource::kSampleLayout) {
    }
    AudioSourceConfig(SINT channelCount, SINT samplingRate)
        : AudioSignal(AudioSource::kSampleLayout, channelCount, samplingRate) {
    }

    using AudioSignal::setChannelCount;
    using AudioSignal::resetChannelCount;

    using AudioSignal::setSamplingRate;
    using AudioSignal::resetSamplingRate;
};

typedef QSharedPointer<AudioSource> AudioSourcePointer;

} // namespace Mixxx
