// readaheadmanager.cpp
// Created 8/2/2009 by RJ Ryan (rryan@mit.edu)

#include "engine/readaheadmanager.h"
#include "sampleutil.h"
#include "util/math.h"
#include "util/defs.h"
#include "engine/loopingcontrol.h"
#include "engine/ratecontrol.h"
#include "cachingreader.h"

ReadAheadManager::ReadAheadManager(QObject*pParent)
        : QObject(pParent),
          m_pCrossFadeBuffer(new CSAMPLE[MAX_BUFFER_LEN]) {
    // For testing only: ReadAheadManagerMock
}
ReadAheadManager::ReadAheadManager(CachingReader* pReader,  LoopingControl* pLoopingControl,QObject*pParent) 
        : QObject(pParent),
          m_pLoopingControl(pLoopingControl),
          m_pReader(pReader),
          m_pCrossFadeBuffer(new CSAMPLE[MAX_BUFFER_LEN]) {
    DEBUG_ASSERT(m_pLoopingControl);
    DEBUG_ASSERT(m_pReader);
    std::fill_n(m_pCrossFadeBuffer,MAX_BUFFER_LEN,0);
}

ReadAheadManager::~ReadAheadManager() { delete[] m_pCrossFadeBuffer; }
int ReadAheadManager::getNextSamples(double dRate, CSAMPLE* buffer, int requested_samples) {
    if (!even(requested_samples)) {
        qDebug() << "ERROR: Non-even requested_samples to ReadAheadManager::getNextSamples";
        requested_samples--;
    }
    auto in_reverse = dRate < 0;
    auto start_sample = m_iCurrentPosition;
    //qDebug() << "start" << start_sample << requested_samples;
    auto samples_needed = requested_samples;
    auto  base_buffer = buffer;
    // A loop will only limit the amount we can read in one shot.
    const double loop_trigger = m_pLoopingControl->nextTrigger(
            dRate, m_iCurrentPosition, 0, 0);
    bool loop_active = loop_trigger != kNoTrigger;
    int preloop_samples = 0;

    if (loop_active) {
        int samples_available = in_reverse ? m_iCurrentPosition - loop_trigger : loop_trigger - m_iCurrentPosition;
        if (samples_available < 0) {
            samples_needed = 0;
        } else {
            preloop_samples = samples_available;
            samples_needed = math_clamp(samples_needed, 0, samples_available);
        }
    }
    if (in_reverse) { start_sample = m_iCurrentPosition - samples_needed; }
    // Sanity checks.
    if (samples_needed < 0) {
        qDebug() << "Need negative samples in ReadAheadManager::getNextSamples. Ignoring read";
        return 0;
    }
    int samples_read = m_pReader->read(start_sample, samples_needed, base_buffer);
    if (samples_read != samples_needed) { qDebug() << "didn't get what we wanted" << samples_read << samples_needed; }

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
        const auto loop_target = m_pLoopingControl-> process(dRate, m_iCurrentPosition, 0, 0);
        if (loop_target != kNoTrigger) {
            m_iCurrentPosition = loop_target;
            int loop_read_position = m_iCurrentPosition + (in_reverse ? preloop_samples : -preloop_samples);
            int looping_samples_read = m_pReader->read(loop_read_position, samples_read, m_pCrossFadeBuffer);
            if (looping_samples_read != samples_read) {qDebug() << "ERROR: Couldn't get all needed samples for crossfade.";}
            // do crossfade from the current buffer into the new loop beginning
            if (samples_read) { // avoid division by zero
                SampleUtil::linearCrossfadeBuffers(base_buffer, base_buffer, m_pCrossFadeBuffer, samples_read);
            }
        }
    }
    // Reverse the samples in-place
    if (in_reverse) { SampleUtil::reverse(base_buffer, samples_read); }
    //qDebug() << "read" << m_iCurrentPosition << samples_read;
    return samples_read;
}
void ReadAheadManager::addRateControl(RateControl* pRateControl) {m_pRateControl = pRateControl;}
// Not thread-save, call from engine thread only
void ReadAheadManager::notifySeek(int iSeekPosition) {
    m_iCurrentPosition = iSeekPosition;
    m_readAheadLog.clear();
}
void ReadAheadManager::hintReader(double dRate, HintVector* pHintList) {
    auto in_reverse = dRate < 0;
    Hint current_position;
    // SoundTouch can read up to 2 chunks ahead. Always keep 2 chunks ahead in
    // cache.
    auto length_to_cache = 2 * CachingReaderChunk::kSamples;
    current_position.length = length_to_cache;
    current_position.sample = in_reverse ?
            m_iCurrentPosition - length_to_cache :
            m_iCurrentPosition;
    // If we are trying to cache before the start of the track,
    // Then we don't need to cache because it's all zeros!
    if (current_position.sample < 0 &&
        current_position.sample + current_position.length < 0)
        return;
    // top priority, we need to read this data immediately
    current_position.priority = 1;
    pHintList->append(current_position);
}

// Not thread-save, call from engine thread only
void ReadAheadManager::addReadLogEntry(double vpps, double vppe) {
    auto newEntry = ReadLogEntry(vpps, vppe);
    if (m_readAheadLog.size() > 0) {
        auto &last = m_readAheadLog.last();
        if (last.merge(newEntry)) return;
    }
    m_readAheadLog.append(newEntry);
}
// Not thread-save, call from engine thread only
int ReadAheadManager::getEffectiveVirtualPlaypositionFromLog(double currentVirtualPlayposition, double numConsumedSamples) {
    if (numConsumedSamples == 0)  return currentVirtualPlayposition;
    if (m_readAheadLog.size() == 0) {
        // No log entries to read from.
        qDebug() << this << "No read ahead log entries to read from. Case not currently handled.";
        // TODO(rryan) log through a stats pipe eventually
        return currentVirtualPlayposition;
    }
    auto virtualPlayposition = 0.0;
    auto shouldNotifySeek = false;
    auto direction = true;
    while (m_readAheadLog.size() > 0 && numConsumedSamples > 0) {
        ReadLogEntry& entry = m_readAheadLog.first();
        direction = entry.direction();
        // Notify EngineControls that we have taken a seek.
        if (shouldNotifySeek) {
            m_pLoopingControl->notifySeek(entry.m_vpps);
            if (m_pRateControl) m_pRateControl->notifySeek(entry.m_vpps);
        }
        auto consumed = entry.consume(numConsumedSamples);
        numConsumedSamples -= consumed;
        // Advance our idea of the current virtual playposition to this
        // ReadLogEntry's start position.
        virtualPlayposition = entry.m_vpps;
        if (entry.length() == 0) m_readAheadLog.removeFirst();
        shouldNotifySeek = true;
    }
    auto result = 0;
    if (direction) {
        result = static_cast<int>(std::floor(virtualPlayposition));
        if (!even(result)) result--;
    } else {
        result = static_cast<int>(std::ceil(virtualPlayposition));
        if (!even(result)) result++;
    }
    return result;
}
ReadAheadManager::ReadLogEntry::ReadLogEntry(double vpps, double vppe)
 : m_vpps(vpps),
   m_vppe(vppe){
}
bool ReadAheadManager::ReadLogEntry::direction() const {
    // NOTE(rryan): We try to avoid 0-length ReadLogEntry's when
    // possible but they have happened in the past. We treat 0-length
    // ReadLogEntry's as forward reads because this prevents them from
    // being interpreted as a seek in the common case.
    return m_vpps <= m_vppe;
}
double ReadAheadManager::ReadLogEntry::length() const
{
    return std::abs(m_vppe - m_vpps);
}

// Moves the start position forward or backward (depending on
// direction()) by numSamples. Returns the total number of samples
// consumed. Caller should check if length() is 0 after consumption in
// order to expire the ReadLogEntry.
double ReadAheadManager::ReadLogEntry::consume(double numSamples)
{
    auto available = math_min(numSamples, length());
    m_vpps += (direction() ? 1 : -1) * available;
    return available;
}
bool ReadAheadManager::ReadLogEntry::merge(const ReadLogEntry& other) {
    // Allow 0-length ReadLogEntry's to merge regardless of their
    // direction if they have the right start point.
    if ((!other.length() || direction() == other.direction()) &&
        m_vppe == other.m_vpps) {
        m_vppe = other.m_vppe;
        return true;
    }
    return false;
}
