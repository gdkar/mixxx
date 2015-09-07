#ifndef MIXXX_AUDIOSOURCE_H
#define MIXXX_AUDIOSOURCE_H

#include "urlresource.h"

#include "util/assert.h"
#include "util/defs.h"
#include "util/types.h"

#include <QSharedPointer>

class SampleBuffer;
namespace Mixxx {

// forward declaration(s)
struct AudioSourceConfig;
// Common interface and base class for audio sources.
//
// Both the number of channels and the frame rate must
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
class AudioSource: public UrlResource {
public:
    static const SINT kChannelCountMono = 1;
    static const SINT kChannelCountStereo = 2;

    // Returns the number of channels. The number of channels
    // must be constant over time.
    virtual  SINT getChannelCount() const;
    // Returns the number of frames per second. This equals
    // the number samples for each channel per second, which
    // must be uniform among all channels. The frame rate
    // must be constant over time.
    virtual SINT getFrameRate() const;
    virtual bool isValid() const;
    // Returns the total number of frames.
    virtual SINT getFrameCount() const;
    virtual bool isEmpty() const;
    // The actual duration in seconds.
    // Well defined only for valid files!
    virtual bool hasDuration() const; 
    virtual double getDuration() const;
    // The bitrate is measured in kbit/s (kbps).
    static bool isValidBitrate(SINT bitrate); 
    virtual bool hasBitrate() const;
    // Setting the bitrate is optional when opening a file.
    // The bitrate is not needed for decoding, it is only used
    // for informational purposes.
    virtual SINT getBitrate() const; 
    // Conversion: #frames -> #samples
    virtual SINT frames2samples(SINT frameCount) const;
    // Conversion: #samples -> #frames
    virtual SINT  samples2frames(SINT sampleCount) const;
    // Index of the first sample frame.
    virtual SINT getMinFrameIndex() const;
    // Index of the sample frame following the last
    // sample frame.
    virtual SINT getMaxFrameIndex() const;
    // The sample frame index is valid in the range
    // [getMinFrameIndex(), getMaxFrameIndex()].
    virtual bool isValidFrameIndex(SINT frameIndex) const;
    // Adjusts the current frame seek index:
    // - Precondition: isValidFrameIndex(frameIndex) == true
    //   - Index of first frame: frameIndex = 0
    //   - Index of last frame: frameIndex = getFrameCount() - 1
    // - The seek position in seconds is frameIndex / frameRate()
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
    virtual SINT readSampleFrames( SINT numberOfFrames, CSAMPLE* sampleBuffer) = 0;
    virtual SINT skipSampleFrames( SINT numberOfFrames) ;
    virtual SINT readSampleFrames( SINT numberOfFrames, SampleBuffer* pSampleBuffer);

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
    virtual SINT readSampleFramesStereo( SINT numberOfFrames, CSAMPLE* sampleBuffer, SINT sampleBufferSize);
    virtual SINT readSampleFramesStereo( SINT numberOfFrames, SampleBuffer* pSampleBuffer); 
    // Utility function to clamp the frame index interval
    // [*pMinFrameIndexOfInterval, *pMaxFrameIndexOfInterval)
    // to valid frame indexes. The lower bound is inclusive and
    // the upper bound is exclusive!
    static void clampFrameInterval(
            SINT &pMinFrameIndexOfInterval,
            SINT &pMaxFrameIndexOfInterval,
            SINT maxFrameIndexOfAudioSource);
protected:
    explicit AudioSource(const QUrl& url);
    static bool isValidChannelCount(SINT channelCount);
    virtual  bool hasChannelCount() const;
    virtual void setChannelCount(SINT channelCount);
     static bool isValidFrameRate(SINT frameRate);
    virtual  bool hasFrameRate() const;
    virtual void setFrameRate(SINT frameRate);
    static bool isValidFrameCount(SINT frameCount);
    virtual void setFrameCount(SINT frameCount);
    virtual void setBitrate(SINT bitrate);
    SINT getSampleBufferSize( SINT numberOfFrames, bool readStereoSamples = false) const;
private:
    friend struct AudioSourceConfig;

    static const SINT kChannelCountZero = 0;
    static const SINT kChannelCountDefault = kChannelCountZero;

    static const SINT kFrameRateZero = 0;
    static const SINT kFrameRateDefault = kFrameRateZero;

    static const SINT kFrameCountZero = 0;
    static const SINT kFrameCountDefault = kFrameCountZero;

    // 0-based indexing of sample frames
    static const SINT kFrameIndexMin = 0;

    static const SINT kBitrateZero = 0;
    static const SINT kBitrateDefault = kBitrateZero;

    SINT m_channelCount;
    SINT m_frameRate;
    SINT m_frameCount;

    SINT m_bitrate;
};

// Parameters for configuring audio sources
struct AudioSourceConfig {
    SINT channelCountHint = AudioSource::kChannelCountDefault;
    SINT frameRateHint    = AudioSource::kFrameRateDefault;
};

typedef QSharedPointer<AudioSource> AudioSourcePointer;

} // namespace Mixxx

#endif // MIXXX_AUDIOSOURCE_H
