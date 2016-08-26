// readaheadmanager.cpp
// Created 8/2/2009 by RJ Ryan (rryan@mit.edu)

#include "engine/readaheadmanager.h"

#include "engine/cachingreader.h"
#include "engine/loopingcontrol.h"
#include "engine/ratecontrol.h"
#include "util/defs.h"
#include "util/math.h"
#include "util/sample.h"

ReadAheadManager::ReadAheadManager(QObject *pParent)
        : QObject(pParent),
          m_pLoopingControl(nullptr),
          m_pRateControl(nullptr),
          m_iCurrentPosition(0),
          m_pReader(nullptr),
          m_pCrossFadeBuffer(SampleUtil::alloc(MAX_BUFFER_LEN)) {
    // For testing only: ReadAheadManagerMock
}

ReadAheadManager::ReadAheadManager(CachingReader* pReader,
                                   LoopingControl* pLoopingControl, QObject *pParent)
        : QObject(pParent),
          m_pLoopingControl(pLoopingControl),
          m_pRateControl(nullptr),
          m_iCurrentPosition(0),
          m_pReader(pReader),
          m_pCrossFadeBuffer(SampleUtil::alloc(MAX_BUFFER_LEN))
{
    DEBUG_ASSERT(m_pLoopingControl != nullptr);
    DEBUG_ASSERT(m_pReader != nullptr);
    connect(this,&ReadAheadManager::notifySeek,this,&ReadAheadManager::onNotifySeek);
    connect(this,&ReadAheadManager::notifySeek,pLoopingControl,&LoopingControl::notifySeek);
    SampleUtil::clear(m_pCrossFadeBuffer, MAX_BUFFER_LEN);
}

ReadAheadManager::~ReadAheadManager()
{
    SampleUtil::free(m_pCrossFadeBuffer);
}

SINT ReadAheadManager::getNextSamples(double dRate, CSAMPLE* buffer,
        SINT requested_samples) {
    // TODO(XXX): Remove implicit assumption of 2 channels
    if (!even(requested_samples)) {
        qDebug() << "ERROR: Non-even requested_samples to ReadAheadManager::getNextSamples";
        requested_samples--;
    }

    bool in_reverse = dRate < 0;
    auto start_sample = m_iCurrentPosition;
    //qDebug() << "start" << start_sample << requested_samples;
    auto samples_needed = requested_samples;
    auto base_buffer = buffer;

    // A loop will only limit the amount we can read in one shot.

    const double loop_trigger = m_pLoopingControl->nextTrigger(
            dRate, m_iCurrentPosition, 0, 0);
    auto loop_active = loop_trigger != kNoTrigger;
    auto preloop_samples = SINT{0};

    if (loop_active) {
        SINT samples_available = (in_reverse ?
                m_iCurrentPosition - loop_trigger :
                loop_trigger - m_iCurrentPosition);
        if (samples_available < 0) {
            samples_needed = 0;
        } else {
            preloop_samples = samples_available;
            samples_needed = math_clamp<SINT>(samples_needed, 0, samples_available);
        }
    }
    if (in_reverse) 
        start_sample = m_iCurrentPosition - samples_needed;

    // Sanity checks.
    if (samples_needed < 0) {
        qDebug() << "Need negative samples in ReadAheadManager::getNextSamples. Ignoring read";
        return 0;
    }
    auto samples_read = m_pReader->read(start_sample, in_reverse, samples_needed,
                                       base_buffer);

    if (samples_read != samples_needed) {
        qDebug() << "didn't get what we wanted" << samples_read << samples_needed;
    }
    // Increment or decrement current read-ahead position
    if (in_reverse) {
        addReadLogEntry(m_iCurrentPosition, m_iCurrentPosition - samples_read);
        m_iCurrentPosition -= samples_read;
    } else {
        addReadLogEntry(m_iCurrentPosition, m_iCurrentPosition + samples_read);
        m_iCurrentPosition += samples_read;
    }
    // Activate on this trigger if necessary
    if (loop_active) {
        // LoopingControl makes the decision about whether we should loop or
        // not.
        auto loop_target = m_pLoopingControl->process(dRate, m_iCurrentPosition, 0, 0);
        if (loop_target != kNoTrigger) {
            m_iCurrentPosition = loop_target;
            auto loop_read_position = m_iCurrentPosition + (in_reverse ? preloop_samples : -preloop_samples);
            auto looping_samples_read = m_pReader->read(loop_read_position, in_reverse, samples_read, m_pCrossFadeBuffer);
            if (looping_samples_read != samples_read) {
                qDebug() << "ERROR: Couldn't get all needed samples for crossfade.";
            }
            // do crossfade from the current buffer into the new loop beginning
            if (samples_read) { // avoid division by zero
                SampleUtil::linearCrossfadeBuffers(base_buffer, base_buffer, m_pCrossFadeBuffer, samples_read);
            }
        }
    }
    //qDebug() << "read" << m_iCurrentPosition << samples_read;
    return samples_read;
}
void ReadAheadManager::addRateControl(RateControl* pRateControl)
{
    disconnect(this,&ReadAheadManager::notifySeek,m_pRateControl,&RateControl::notifySeek);
    m_pRateControl = pRateControl;
    connect(this,&ReadAheadManager::notifySeek,m_pRateControl,&RateControl::notifySeek);

}
// Not thread-save, call from engine thread only
void ReadAheadManager::onNotifySeek(double iSeekPosition) {
    m_iCurrentPosition = iSeekPosition;
    m_readAheadLog.clear();
}
void ReadAheadManager::hintReader(double dRate, HintVector* pHintList)
{
    auto in_reverse = dRate < 0;
    Hint current_position;
    // SoundTouch can read up to 2 chunks ahead. Always keep 2 chunks ahead in
    // cache.
    auto length_to_cache = 2 * CachingReaderChunk::kSamples;

    current_position.length = length_to_cache;
    current_position.sample = in_reverse ? m_iCurrentPosition - length_to_cache : m_iCurrentPosition;
    // If we are trying to cache before the start of the track,
    // Then we don't need to cache because it's all zeros!
    if (current_position.sample < 0 && current_position.sample + current_position.length < 0)
        return;
    // top priority, we need to read this data immediately
    current_position.priority = 1;
    pHintList->append(current_position);
}

// Not thread-save, call from engine thread only
void ReadAheadManager::addReadLogEntry(double virtualPlaypositionStart,
                                       double virtualPlaypositionEnd) {
    auto newEntry = ReadLogEntry(virtualPlaypositionStart,virtualPlaypositionEnd);
    if (!m_readAheadLog.empty()) {
        auto& back= m_readAheadLog.back();
        if (back.merge(newEntry))
            return;
    }
    m_readAheadLog.append(newEntry);
}

// Not thread-save, call from engine thread only
SINT ReadAheadManager::getEffectiveVirtualPlaypositionFromLog(double currentVirtualPlayposition,
                                                             double numConsumedSamples) {
    if (numConsumedSamples == 0) {
        return currentVirtualPlayposition;
    }
    if (m_readAheadLog.empty()) {
        // No log entries to read from.
        qDebug() << this << "No read ahead log entries to read from. Case not currently handled.";
        // TODO(rryan) log through a stats pipe eventually
        return currentVirtualPlayposition;
    }
    auto virtualPlayposition = 0.;
    auto shouldNotifySeek = false;
    auto direction = true;
    while (!m_readAheadLog.empty() && numConsumedSamples > 0) {
        auto& entry = m_readAheadLog.front();
        direction = entry.direction();
        // Notify EngineControls that we have taken a seek.
        if (shouldNotifySeek) {
            emit notifySeek(entry.virtualPlaypositionStart);
        }
        auto consumed = entry.consume(numConsumedSamples);
        numConsumedSamples -= consumed;
        // Advance our idea of the current virtual playposition to this
        // ReadLogEntry's start position.
        virtualPlayposition = entry.virtualPlaypositionStart;
        if (entry.length() == 0) {
            // This entry is empty now.
            m_readAheadLog.removeFirst();
        }
        shouldNotifySeek = true;
    }
    auto result = SINT{};
    if (direction) {
        result = static_cast<SINT>(std::floor(virtualPlayposition));
        // TODO(XXX): Remove implicit assumption of 2 channels
        if (!even(result)) {
            result--;
        }
    } else {
        result = static_cast<SINT>(std::ceil(virtualPlayposition));
        // TODO(XXX): Remove implicit assumption of 2 channels
        if (!even(result)) {
            result++;
        }
    }
    return result;
}
