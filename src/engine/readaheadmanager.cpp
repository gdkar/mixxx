// readaheadmanager.cpp
// Created 8/2/2009 by RJ Ryan (rryan@mit.edu)

#include "engine/readaheadmanager.h"
#include "sampleutil.h"
#include "util/math.h"
#include "util/defs.h"
#include "engine/loopingcontrol.h"
#include "engine/ratecontrol.h"
#include "engine/cachingreader.h"

ReadAheadManager::ReadAheadManager()
        : m_pLoopingControl(NULL),
          m_pRateControl(NULL),
          m_dCurrentPosition(0),
          m_pReader(NULL),
          m_pCrossFadeBuffer(SampleUtil::alloc(MAX_BUFFER_LEN)) {
    // For testing only: ReadAheadManagerMock
}

ReadAheadManager::ReadAheadManager(CachingReader* pReader, 
                                   LoopingControl* pLoopingControl) 
        : m_pLoopingControl(pLoopingControl),
          m_pRateControl(NULL),
          m_dCurrentPosition(0),
          m_pReader(pReader),
          m_pCrossFadeBuffer(SampleUtil::alloc(MAX_BUFFER_LEN)) {
    DEBUG_ASSERT(m_pLoopingControl != NULL);
    DEBUG_ASSERT(m_pReader != NULL);
    SampleUtil::clear(m_pCrossFadeBuffer, MAX_BUFFER_LEN);
}

ReadAheadManager::~ReadAheadManager() {SampleUtil::free(m_pCrossFadeBuffer);}
double ReadAheadManager::getNextSamples(double dRate, CSAMPLE* buffer,double requested_samples) {
    requested_samples = static_cast<double>(static_cast<qint64>(requested_samples) &(~1));
//    if (!even(requested_samples)) {
//        qDebug() << "ERROR: Non-even requested_samples to ReadAheadManager::getNextSamples";
//        requested_samples--;
//    }
    bool in_reverse = dRate < 0;
    double start_sample = m_dCurrentPosition;
    //qDebug() << "start" << start_sample << requested_samples;
    double samples_needed = requested_samples;
    CSAMPLE* base_buffer = buffer;
    // A loop will only limit the amount we can read in one shot.
    const double loop_trigger = m_pLoopingControl->nextTrigger(dRate, m_dCurrentPosition, 0, 0);
    bool loop_active = loop_trigger != kNoTrigger;
    double preloop_samples = 0;
    if (loop_active) {
        double samples_available = in_reverse ?m_dCurrentPosition - loop_trigger : loop_trigger - m_dCurrentPosition;
        if (samples_available < 0) {samples_needed = 0;
        } else {
            preloop_samples = samples_available;
            samples_needed = math_clamp(samples_needed, 0.0, samples_available);
        }
    }
    if (in_reverse) {start_sample = m_dCurrentPosition - samples_needed;}
    // Sanity checks.
    if (samples_needed < 0) {
        qDebug() << "Need negative samples in ReadAheadManager::getNextSamples. Ignoring read";
        return 0;
    }
    double samples_read = m_pReader->read(start_sample, samples_needed,base_buffer);
    if (samples_read != samples_needed) {qDebug() << "didn't get what we wanted" << samples_read << samples_needed;}
    // Increment or decrement current read-ahead position
    if (in_reverse) {
        addReadLogEntry(m_dCurrentPosition, m_dCurrentPosition - samples_read);
        m_dCurrentPosition -= samples_read;
    } else {
        addReadLogEntry(m_dCurrentPosition, m_dCurrentPosition + samples_read);
        m_dCurrentPosition += samples_read;
    }
    // Activate on this trigger if necessary
    if (loop_active) {
        // LoopingControl makes the decision about whether we should loop or
        // not.
        const double loop_target = m_pLoopingControl->process(dRate, m_dCurrentPosition, 0, 0);
        if (loop_target != kNoTrigger) {
            m_dCurrentPosition = loop_target;
            double loop_read_position = m_dCurrentPosition + (in_reverse ? preloop_samples : -preloop_samples);
            double looping_samples_read = m_pReader->read(loop_read_position, samples_read, m_pCrossFadeBuffer);
            if (looping_samples_read != samples_read) {qDebug() << "ERROR: Couldn't get all needed samples for crossfade.";}
            // do crossfade from the current buffer into the new loop beginning
            if (samples_read != 0) { // avoid division by zero
                SampleUtil::linearCrossfadeBuffers(base_buffer, base_buffer, m_pCrossFadeBuffer, samples_read);
            }
        }
    }
    // Reverse the samples in-place
    if (in_reverse) {SampleUtil::reverse(base_buffer, samples_read);}
    //qDebug() << "read" << m_dCurrentPosition << samples_read;
    return samples_read;
}
void ReadAheadManager::addRateControl(RateControl* pRateControl) {m_pRateControl = pRateControl;}
// Not thread-save, call from engine thread only
void ReadAheadManager::notifySeek(double dSeekPosition) {
    m_dCurrentPosition = dSeekPosition;
    m_readAheadLog.clear();
    // TODO(XXX) notifySeek on the engine controls. EngineBuffer currently does
    // a fine job of this so it isn't really necessary but eventually I think
    // RAMAN should do this job. rryan 11/2011

    // foreach (EngineControl* pControl, m_sEngineControls) {
    //     pControl->notifySeek(iSeekPosition);
    // }
}

void ReadAheadManager::hintReader(double dRate, HintVector* pHintList) {
    bool in_reverse = dRate < 0;
    Hint current_position;
    // SoundTouch can read up to 2 chunks ahead. Always keep 2 chunks ahead in
    // cache.
    double length_to_cache = 2 * CachingReaderWorker::kSamplesPerChunk;
    current_position.length = length_to_cache;
    current_position.sample = in_reverse ? m_dCurrentPosition - length_to_cache : m_dCurrentPosition;
    // If we are trying to cache before the start of the track,
    // Then we don't need to cache because it's all zeros!
    if (current_position.sample < 0 && current_position.sample + current_position.length < 0) return;
    // top priority, we need to read this data immediately
    current_position.priority = 1;
    pHintList->append(current_position);
}

// Not thread-save, call from engine thread only
void ReadAheadManager::addReadLogEntry(double virtualPlaypositionStart, double virtualPlaypositionEndNonInclusive) {
    ReadLogEntry newEntry(virtualPlaypositionStart,
                          virtualPlaypositionEndNonInclusive);
    if (m_readAheadLog.size() > 0) {
        ReadLogEntry& last = m_readAheadLog.last();
        if (last.merge(newEntry)) {return;}
    }
    m_readAheadLog.append(newEntry);
}

// Not thread-save, call from engine thread only
double ReadAheadManager::getEffectiveVirtualPlaypositionFromLog(double currentVirtualPlayposition,
                                                             double numConsumedSamples) {
    if (numConsumedSamples == 0) {return currentVirtualPlayposition;}
    if (m_readAheadLog.size() == 0) {
        // No log entries to read from.
        qDebug() << this << "No read ahead log entries to read from. Case not currently handled.";
        // TODO(rryan) log through a stats pipe eventually
        return currentVirtualPlayposition;
    }
    double virtualPlayposition = 0;
    bool shouldNotifySeek = false;
    bool direction = true;
    while (m_readAheadLog.size() > 0 && numConsumedSamples > 0) {
        ReadLogEntry& entry = m_readAheadLog.first();
        direction = entry.direction();
        // Notify EngineControls that we have taken a seek.
        if (shouldNotifySeek) {
            m_pLoopingControl->notifySeek(entry.virtualPlaypositionStart);
            if (m_pRateControl) {m_pRateControl->notifySeek(entry.virtualPlaypositionStart);}
        }
        double consumed = entry.consume(numConsumedSamples);
        numConsumedSamples -= consumed;
        // Advance our idea of the current virtual playposition to this
        // ReadLogEntry's start position.
        virtualPlayposition = entry.virtualPlaypositionStart;
        // This entry is empty now.
        if (entry.length() == 0) {m_readAheadLog.removeFirst();}
        shouldNotifySeek = true;
    }
    double result = 0;
    if (direction) {
        result = static_cast<double>(static_cast<qint64>(floor(virtualPlayposition))&(~1));
    } else {
        result = static_cast<double>(static_cast<qint64>(ceil(virtualPlayposition+0.5))&(~1));
    }
    return result;
}
