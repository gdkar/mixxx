#include "engine/enginebufferscalelinear.h"

#include <QtDebug>

#include "track/keyutils.h"
#include "util/assert.h"
#include "util/math.h"
#include "util/sample.h"

EngineBufferScaleLinear::EngineBufferScaleLinear(ReadAheadManager *pReadAheadManager, QObject *pParent)
    : EngineBufferScale(pReadAheadManager,pParent),
      m_bufferInt(SampleUtil::alloc(kiLinearScaleReadAheadLength)),
      m_bufferIntSize(0),
      m_bClear(false),
      m_dRate(1.0),
      m_dOldRate(1.0),
      m_dCurrentFrame(0.0),
      m_dNextFrame(0.0) {
    m_floorSampleOld[0] = 0.0;
    m_floorSampleOld[1] = 0.0;
    SampleUtil::clear(m_bufferInt, kiLinearScaleReadAheadLength);
}

EngineBufferScaleLinear::~EngineBufferScaleLinear() {
    SampleUtil::free(m_bufferInt);
}

void EngineBufferScaleLinear::setScaleParameters(double base_rate,
                                                 double &pTempoRatio,
                                                 double &pPitchRatio) {
    Q_UNUSED(pPitchRatio);

    m_dOldRate = m_dRate;
    m_dRate = base_rate * pTempoRatio;
}

void EngineBufferScaleLinear::clear()
{
    m_bClear = true;
    // Clear out buffer and saved sample data
    m_bufferIntSize = 0;
    m_dNextFrame = 0;
    m_floorSampleOld[0] = 0;
    m_floorSampleOld[1] = 0;
}

// laurent de soras - punked from musicdsp.org (mad props)
inline float hermite4(float frac_pos, float xm1, float x0, float x1, float x2)
{
    auto c = (x1 - xm1) * 0.5f;
    auto v = x0 - x1;
    auto w = c + v;
    auto a = w + v + (x2 - x0) * 0.5f;
    auto b_neg = w + a;
    return ((((a * frac_pos) - b_neg) * frac_pos + c) * frac_pos + x0);
}

// Determine if we're changing directions (scratching) and then perform
// a stretch
double EngineBufferScaleLinear::scaleBuffer(
        CSAMPLE* pOutputBuffer,
        SINT iOutputBufferSize) {
    if (iOutputBufferSize == 0) {
        return 0.0;
    }

    if (m_bClear) {
        m_dOldRate = m_dRate;  // If cleared, don't interpolate rate.
        m_bClear = false;
    }
    auto rate_add_old = float(m_dOldRate);  // Smoothly interpolate to new playback rate
    auto rate_add_new = float(m_dRate);
    auto frames_read = SINT{0};

    if (rate_add_new * rate_add_old < 0) {
        // Direction has changed!
        // calculate half buffer going one way, and half buffer going
        // the other way.

        // first half: rate goes from old rate to zero
        m_dOldRate = rate_add_old;
        m_dRate = 0.0;
        frames_read += do_scale(pOutputBuffer, getAudioSignal().samples2frames(iOutputBufferSize));

        // reset m_floorSampleOld in a way as we were coming from
        // the other direction
        auto iNextSample = getAudioSignal().frames2samples(static_cast<SINT>(ceil(m_dNextFrame)));
        if (iNextSample + 1 < m_bufferIntSize) {
            m_floorSampleOld[0] = m_bufferInt[iNextSample];
            m_floorSampleOld[1] = m_bufferInt[iNextSample + 1];
        }

        // if the buffer has extra samples, do a read so RAMAN ends up back where
        // it should be
        auto iCurSample = getAudioSignal().frames2samples(static_cast<SINT>(ceil(m_dCurrentFrame)));
        auto extra_samples = m_bufferIntSize - iCurSample - getAudioSignal().getChannelCount();
        if (extra_samples > 0) {
            if (extra_samples % getAudioSignal().getChannelCount() != 0) {
                // extra samples should include the whole frame
                extra_samples -= extra_samples % getAudioSignal().getChannelCount();
                extra_samples += getAudioSignal().getChannelCount();
            }
            //qDebug() << "extra samples" << extra_samples;

            auto next_samples_read = m_pReadAheadManager->getNextSamples(
                    rate_add_new, m_bufferInt, extra_samples);
            frames_read += getAudioSignal().samples2frames(next_samples_read);
        }
        // force a buffer read:
        m_bufferIntSize = 0;
        // make sure the indexes stay correct for interpolation
        // TODO() Why we do not swap current and Next?
        m_dCurrentFrame = 0.0 - m_dCurrentFrame + floor(m_dCurrentFrame);
        m_dNextFrame = 1.0 - (m_dNextFrame - floor(m_dNextFrame));

        // second half: rate goes from zero to new rate
        m_dOldRate = 0.0;
        m_dRate = rate_add_new;
        // pass the address of the frame at the halfway point
        auto frameOffset =  getAudioSignal().samples2frames(iOutputBufferSize) / 2;
        auto sampleOffset = getAudioSignal().frames2samples(frameOffset);
        frames_read += do_scale(pOutputBuffer + sampleOffset, iOutputBufferSize - sampleOffset);
    } else {
        frames_read += do_scale(pOutputBuffer, iOutputBufferSize);
    }
    return frames_read;
}

SINT EngineBufferScaleLinear::do_copy(CSAMPLE* buf, SINT buf_size) {
    SINT samples_needed = buf_size;
    CSAMPLE* write_buf = buf;
    // Use up what's left of the internal buffer.
    auto iNextFrame = static_cast<SINT>(ceil(m_dNextFrame));
    auto iNextSample = math_max<SINT>(getAudioSignal().frames2samples(iNextFrame), 0);
    auto readSize = math_min<SINT>(m_bufferIntSize - iNextSample, samples_needed);
    if (readSize > 0) {
        SampleUtil::copy(write_buf, &m_bufferInt[iNextSample], readSize);
        samples_needed -= readSize;
        write_buf += readSize;
    }
    // Protection against infinite read loops when (for example) we are
    // reading from a broken file.
    auto read_failed_count = 0;
    // We need to repeatedly call the RAMAN because the RAMAN does not bend
    // over backwards to satisfy our request. It assumes you will continue
    // to call getNextSamples until you receive the number of samples you
    // wanted.
    while (samples_needed > 0) {
        auto read_size = m_pReadAheadManager->getNextSamples(m_dRate, write_buf,
                samples_needed);
        if (read_size == 0) {
            if (++read_failed_count > 1) {
                break;
            } else {
                continue;
            }
        }
        samples_needed -= read_size;
        write_buf += read_size;
    }

    // Instead of counting how many samples we got from the internal buffer
    // and the RAMAN calls, just measure the difference between what we
    // requested and what we still need.
    auto read_samples = buf_size - samples_needed;
    // Zero the remaining samples if we didn't fill them.
    SampleUtil::clear(write_buf, samples_needed);
    // update our class members so next time we need to scale it's ok. we do
    // blow away the fractional sample position here
    m_bufferIntSize = 0; // force buffer read
    m_dNextFrame = 0;
    if (read_samples > 1) {
        m_floorSampleOld[0] = buf[read_samples - 2];
        m_floorSampleOld[1] = buf[read_samples - 1];
    }
    return read_samples;
}

// Stretch a specified buffer worth of audio using linear interpolation
SINT EngineBufferScaleLinear::do_scale(CSAMPLE* buf, SINT buf_size) {
    auto rate_old = float(m_dOldRate);
    auto rate_new = float(m_dRate);
    auto rate_diff = rate_new - rate_old;
    // Update the old base rate because we only need to
    // interpolate/ramp up the pitch changes once.
    m_dOldRate = m_dRate;

    // Determine position in read_buffer to start from. (This is always 0 with
    // the new EngineBuffer implementation)

    // We special case direction change in the calling function, so this
    // shouldn't happen
    DEBUG_ASSERT_AND_HANDLE(rate_new * rate_old >= 0) {
        // We cannot change direction here.
        qDebug() << "EBSL::do_scale() can't change direction";
        rate_old = 0;
    }

    // Special case -- no scaling needed!
    if (rate_diff == 0 && (rate_new == 1.0 || rate_new == -1.0)) {
        return do_copy(buf, buf_size);
    }

    // Simulate the loop to estimate how many frames we need
    auto frames = 0.;
    auto bufferSizeFrames = getAudioSignal().samples2frames(buf_size);
    auto rate_delta = rate_diff / bufferSizeFrames;
    // use Gaussian sum formula (n(n+1))/2 for
    //for (int j = 0; j < bufferSizeFrames; ++j) {
    //    frames += (j * rate_delta) + rate_old;
    //}
    frames = (bufferSizeFrames - 1) * bufferSizeFrames / 2.0;
    frames *= rate_delta;
    frames += rate_old * bufferSizeFrames;
    frames = fabs(frames);

    // Intentional integer rounding: increases by one if the fractional part of
    // m_dNextFrame and frames are greater than one"
    auto unscaled_frames_needed = static_cast<SINT>(frames +
            m_dNextFrame - floor(m_dNextFrame));

    auto read_failed_count = 0;
    CSAMPLE floor_sample[2];
    CSAMPLE ceil_sample[2];

    floor_sample[0] = 0;
    floor_sample[1] = 0;
    ceil_sample[0] = 0;
    ceil_sample[1] = 0;

    auto frames_read = SINT{0};
    auto i = SINT{0};
    //int screwups_debug = 0;

    auto rate_add = std::abs(rate_old);
    auto rate_delta_abs =
            rate_old < 0 || rate_new < 0 ? -rate_delta : rate_delta;

    // Hot frame loop
    while (i < buf_size) {
        // shift indicies
        m_dCurrentFrame = m_dNextFrame;

        // Because our index is a float value, we're going to be interpolating
        // between two samples, a lower (prev) and upper (cur) sample.
        // If the lower sample is off the end of the buffer (values between
        // -.999 and 0), load it from the saved globals.

        // The first bounds check (< m_bufferIntSize) is probably not needed.

        auto currentFrameFloor = static_cast<SINT>(floor(m_dCurrentFrame));

        if (currentFrameFloor < 0) {
            // we have advanced to a new buffer in the previous run,
            // but the floor still points to the old buffer
            // so take the cached sample, this happens on slow rates
            floor_sample[0] = m_floorSampleOld[0];
            floor_sample[1] = m_floorSampleOld[1];
            ceil_sample[0] = m_bufferInt[0];
            ceil_sample[1] = m_bufferInt[1];
        } else if (getAudioSignal().frames2samples(currentFrameFloor) + 3 < m_bufferIntSize) {
            // take floor_sample form the buffer of the previous run
            floor_sample[0] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor)];
            floor_sample[1] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor) + 1];
            ceil_sample[0] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor) + 2];
            ceil_sample[1] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor) + 3];
        } else {
            // if we don't have the ceil_sample in buffer, load some more

            if (getAudioSignal().frames2samples(currentFrameFloor) + 1 < m_bufferIntSize) {
                // take floor_sample form the buffer of the previous run
                floor_sample[0] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor)];
                floor_sample[1] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor) + 1];
            }

            do {
                auto old_bufsize = m_bufferIntSize;
                if (unscaled_frames_needed == 0) {
                    // protection against infinite loop
                    // This may happen due to double precision issues
                    ++unscaled_frames_needed;
                }

                auto samples_to_read = math_min<SINT>(
                        kiLinearScaleReadAheadLength,
                        getAudioSignal().frames2samples(unscaled_frames_needed));

                m_bufferIntSize = m_pReadAheadManager->getNextSamples(
                        rate_new == 0 ? rate_old : rate_new,
                        m_bufferInt, samples_to_read);

                if (m_bufferIntSize == 0) {
                    if (++read_failed_count > 1) {
                        break;
                    } else {
                        continue;
                    }
                }

                frames_read += getAudioSignal().samples2frames(m_bufferIntSize);
                unscaled_frames_needed -= getAudioSignal().samples2frames(m_bufferIntSize);

                // adapt the m_dCurrentFrame the index of the new buffer
                m_dCurrentFrame -= getAudioSignal().samples2frames(old_bufsize);
                currentFrameFloor = static_cast<SINT>(floor(m_dCurrentFrame));
            } while (getAudioSignal().frames2samples(currentFrameFloor) + 3 >= m_bufferIntSize);

            // I guess?
            if (read_failed_count > 1) {
                break;
            }

            // Now that the buffer is up to date, we can get the value of the sample
            // at the floor of our position.
            if (currentFrameFloor >= 0) {
                // the previous position is in the new buffer
                floor_sample[0] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor)];
                floor_sample[1] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor) + 1];
            }
            ceil_sample[0] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor) + 2];
            ceil_sample[1] = m_bufferInt[getAudioSignal().frames2samples(currentFrameFloor) + 3];
        }

        // For the current index, what percentage is it
        // between the previous and the next?
        auto frac = static_cast<CSAMPLE>(m_dCurrentFrame) - currentFrameFloor;
        // Perform linear interpolation
        buf[i] = floor_sample[0] + frac * (ceil_sample[0] - floor_sample[0]);
        buf[i + 1] = floor_sample[1] + frac * (ceil_sample[1] - floor_sample[1]);

        m_floorSampleOld[0] = floor_sample[0];
        m_floorSampleOld[1] = floor_sample[1];
        // increment the index for the next loop
        m_dNextFrame = m_dCurrentFrame + rate_add;
        // Smooth any changes in the playback rate over one buf_size
        // samples. This prevents the change from being discontinuous and helps
        // improve sound quality.
        rate_add += rate_delta_abs;
        i += getAudioSignal().getChannelCount();
    }
    SampleUtil::clear(&buf[i], buf_size - i);
    return frames_read;
}
