#include "analyzer/analyzerqueue.h"

#include <typeinfo>

#include <QtDebug>
#include <QMutexLocker>

#ifdef __VAMP__
#include "analyzer/analyzerbeats.h"
#include "analyzer/analyzerkey.h"
#include "analyzer/vamp/vampanalyzer.h"
#endif
#include "analyzer/analyzergain.h"
#include "analyzer/analyzerebur128.h"
#include "analyzer/analyzerwaveform.h"
#include "library/trackcollection.h"
#include "mixer/playerinfo.h"
#include "sources/soundsourceproxy.h"
#include "track/track.h"
#include "util/compatibility.h"
#include "util/event.h"
#include "util/timer.h"
#include "util/trace.h"

// Measured in 0.1%,
// 0 for no progress during finalize
// 1 to display the text "finalizing"
// 100 for 10% step after finalize
#define FINALIZE_PROMILLE 1

namespace {
    // Analysis is done in blocks.
    // We need to use a smaller block size, because on Linux the AnalyzerQueue
    // can starve the CPU of its resources, resulting in xruns. A block size
    // of 4096 frames per block seems to do fine.
    const SINT kAnalysisChannels = mixxx::AudioSource::kChannelCountStereo;
    const SINT kAnalysisFramesPerBlock = 4096;
    const SINT kAnalysisSamplesPerBlock =
            kAnalysisFramesPerBlock * kAnalysisChannels;
} // anonymous namespace

AnalyzerQueue::AnalyzerQueue(TrackCollection* pTrackCollection)
        : m_aq(),
          m_exit(false),
          m_aiCheckPriorities(false),
          m_sampleBuffer(kAnalysisSamplesPerBlock),
          m_tioq(),
          m_qm(),
          m_qwait(),
          m_queue_size(0)
{
    Q_UNUSED(pTrackCollection);
    connect(this, SIGNAL(updateProgress()),
            this, SLOT(slotUpdateProgress()));
}

AnalyzerQueue::~AnalyzerQueue()
{
    stop();
    m_progressInfo.sema.release();
    wait(); //Wait until thread has actually stopped before proceeding.

    for(auto & an : as_const(m_aq)) {
        delete an;
    }
    //qDebug() << "AnalyzerQueue::~AnalyzerQueue()";
}

void AnalyzerQueue::addAnalyzer(Analyzer* an)
{
    m_aq.push_back(an);
}

// This is called from the AnalyzerQueue thread
bool AnalyzerQueue::isLoadedTrackWaiting(TrackPointer analysingTrack)
{
    auto && info = PlayerInfo::instance();
    TrackPointer pTrack;
    auto trackWaiting = false;
    QList<TrackPointer> progress100List;
    QList<TrackPointer> progress0List;

    m_qm.lock();
    QMutableListIterator<TrackPointer> it(m_tioq);
    while (it.hasNext()) {
        auto & pTrack = it.next();
        if (!pTrack) {
            it.remove();
            continue;
        }
        if (!trackWaiting) {
            trackWaiting = info.isTrackLoaded(pTrack);
        }
        // try to load waveforms for all new tracks first
        // and remove them from queue if already analysed
        // This avoids waiting for a running analysis for those tracks.
        auto progress = pTrack->getAnalyzerProgress();
        if (progress < 0) {
            // Load stored analysis
            auto processTrack = false;
            for(auto a : as_const(m_aq)) {
                if (!a->isDisabledOrLoadStoredSuccess(pTrack)) {
                    processTrack = true;
                }
            }
            if (!processTrack) {
                progress100List.append(pTrack);
                it.remove(); // since pTrack is a reference it is invalid now.
            } else {
                progress0List.append(pTrack);
            }
        } else if (progress == 1000) {
            it.remove();
        }
    }

    m_qm.unlock();

    // update progress after unlock to avoid a deadlock
    for(auto pTrack: as_const(progress100List))
        emitUpdateProgress(pTrack, 1000);
    for(auto pTrack: as_const(progress0List))
        emitUpdateProgress(pTrack, 0);

    if (info.isTrackLoaded(analysingTrack)) {
        return false;
    }
    return trackWaiting;
}

// This is called from the AnalyzerQueue thread
TrackPointer AnalyzerQueue::dequeueNextBlocking()
{
    auto _lock = QMutexLocker(&m_qm);
    if (m_tioq.isEmpty()) {
        Event::end("AnalyzerQueue process");
        m_qwait.wait(&m_qm);
        Event::start("AnalyzerQueue process");

        if (m_exit) {
            return TrackPointer();
        }
    }
    auto && info = PlayerInfo::instance();
    TrackPointer pLoadTrack;
    QMutableListIterator<TrackPointer> it(m_tioq);
    while (it.hasNext()) {
        if(auto& pTrack = it.next()) {
            // Prioritize tracks that are loaded.
            if (info.isTrackLoaded(pTrack)) {
                qDebug() << "Prioritizing" << pTrack->getTitle() << pTrack->getLocation();
                pLoadTrack = pTrack;
                it.remove();
                break;
            }
        }else{
            it.remove();
        }
    }
    if (!pLoadTrack && !m_tioq.isEmpty()) {
        // no prioritized track found, use head track
        pLoadTrack = m_tioq.dequeue();
    }
    if (pLoadTrack) {
        qDebug() << "Analyzing" << pLoadTrack->getTitle() << pLoadTrack->getLocation();
    }
    // pTrack might be NULL, up to the caller to check.
    return pLoadTrack;
}

// This is called from the AnalyzerQueue thread
bool AnalyzerQueue::doAnalysis(TrackPointer tio, mixxx::AudioSourcePointer pAudioSource)
{
    QTime progressUpdateInhibitTimer;
    progressUpdateInhibitTimer.start(); // Inhibit Updates for 60 milliseconds

    auto frameIndex = pAudioSource->seekSampleFrame(pAudioSource->getMinFrameIndex());
    auto dieflag = false;
    auto cancelled = false;
    do {
        ScopedTimer t("AnalyzerQueue::doAnalysis block");

        DEBUG_ASSERT(frameIndex < pAudioSource->getMaxFrameIndex());
        auto framesRemaining = pAudioSource->getMaxFrameIndex() - frameIndex;
        auto framesToRead = std::min(kAnalysisFramesPerBlock, framesRemaining);
        DEBUG_ASSERT(0 < framesToRead);

        auto framesRead =
                pAudioSource->readSampleFramesStereo(
                        kAnalysisFramesPerBlock,
                        &m_sampleBuffer);
//        DEBUG_ASSERT(framesRead <= framesToRead);
        frameIndex += framesRead;
//        DEBUG_ASSERT(pAudioSource->isValidFrameIndex(frameIndex));

        // To compare apples to apples, let's only look at blocks that are
        // the full block size.
        if (kAnalysisFramesPerBlock == framesRead) {
            // Complete analysis block of audio samples has been read.
            for(auto an : as_const(m_aq)) {
                //qDebug() << typeid(*an).name() << ".process()";
                an->process(m_sampleBuffer.data(), m_sampleBuffer.size());
                //qDebug() << "Done " << typeid(*an).name() << ".process()";
            }
        } else {
            // Partial analysis block of audio samples has been read.
            // This should only happen at the end of an audio stream,
            // otherwise a decoding error must have occurred.
            if (frameIndex < pAudioSource->getMaxFrameIndex()) {
                // EOF not reached -> Maybe a corrupt file?
/*                qWarning() << "Failed to read sample data from file:"
                        << tio->getLocation()
                        << "@" << frameIndex;*/
                if (0 >= framesRead) {
                    // If no frames have been read then abort the analysis.
                    // Otherwise we might get stuck in this loop forever.
                    dieflag = true; // abort
                    cancelled = false; // completed, no retry
                }
            }
        }

        // emit progress updates
        // During the doAnalysis function it goes only to 100% - FINALIZE_PERCENT
        // because the finalize functions will take also some time
        //fp div here prevents insane signed overflow
//        DEBUG_ASSERT(pAudioSource->isValidFrameIndex(frameIndex));
        auto frameProgress = double(frameIndex) / double(pAudioSource->getMaxFrameIndex());
        auto progressPromille = int(frameProgress * (1000 - FINALIZE_PROMILLE));

        if (m_progressInfo.track_progress != progressPromille) {
            if (progressUpdateInhibitTimer.elapsed() > 60) {
                // Inhibit Updates for 60 milliseconds
                emitUpdateProgress(tio, progressPromille);
                progressUpdateInhibitTimer.start();
            }
        }
        // Since this is a background analysis queue, we should co-operatively
        // yield every now and then to try and reduce CPU contention. The
        // analyzer queue is CPU intensive so we want to get out of the way of
        // the audio callback thread.
        //QThread::yieldCurrentThread();
        //QThread::usleep(10);

        // has something new entered the queue?
        if (m_aiCheckPriorities.exchange(false)) {
            if (isLoadedTrackWaiting(tio)) {
                qDebug() << "Interrupting analysis to give preference to a loaded track.";
                dieflag = true;
                cancelled = true;
            }
        }
        if (m_exit) {
            dieflag = true;
            cancelled = true;
        }

        // Ignore blocks in which we decided to bail for stats purposes.
        if (dieflag || cancelled) {
            t.cancel();
        }
    } while (!dieflag && (frameIndex < pAudioSource->getMaxFrameIndex()));

    return !cancelled; //don't return !dieflag or we might reanalyze over and over
}

void AnalyzerQueue::stop()
{
    m_exit = true;
    QMutexLocker locker(&m_qm);
    m_qwait.wakeAll();
}

void AnalyzerQueue::run()
{
    static auto id = 0u; // the id of this thread, for debugging purposes
    QThread::currentThread()->setObjectName(QString("AnalyzerQueue %1").arg(++id));

    // If there are no analyzers, don't waste time running.
    if (m_aq.empty())
        return;

    m_progressInfo.current_track.reset();
    m_progressInfo.track_progress = 0;
    m_progressInfo.queue_size = 0;
    m_progressInfo.sema.release(); // Initialize with one

    while (!m_exit) {
        auto nextTrack = dequeueNextBlocking();
        // It's important to check for m_exit here in case we decided to exit
        // while blocking for a new track.
        if (m_exit) {
            break;
        }
        // If the track is NULL, try to get the next one.
        // Could happen if the track was queued but then deleted.
        // Or if dequeueNextBlocking is unblocked by exit == true
        if (!nextTrack) {
            emptyCheck();
            continue;
        }
        Trace trace("AnalyzerQueue analyzing track");
        // Get the audio
        SoundSourceProxy soundSourceProxy(nextTrack);
        mixxx::AudioSourceConfig audioSrcCfg;
        audioSrcCfg.setChannelCount(kAnalysisChannels);
        mixxx::AudioSourcePointer pAudioSource(soundSourceProxy.openAudioSource(audioSrcCfg));
        if (!pAudioSource) {
            qWarning() << "Failed to open file for analyzing:" << nextTrack->getLocation();
            emptyCheck();
            continue;
        }
        auto processTrack = false;
        for(auto an : as_const(m_aq)) {
            // Make sure not to short-circuit initialize(...)
            if (an->initialize(nextTrack, pAudioSource->getSamplingRate(), pAudioSource->getFrameCount() * kAnalysisChannels)) {
                processTrack = true;
            }
        }
        {
            QMutexLocker locker(&m_qm);
            m_queue_size = m_tioq.size();
        }

        if (processTrack) {
            emitUpdateProgress(nextTrack, 0);
            auto completed = doAnalysis(nextTrack, pAudioSource);
            if (!completed) {
                // This track was cancelled
                for(auto an : as_const(m_aq))
                    an->cleanup(nextTrack);
                queueAnalyseTrack(nextTrack);
                emitUpdateProgress(nextTrack, 0);
            } else {
                // 100% - FINALIZE_PERCENT finished
                emitUpdateProgress(nextTrack, 1000 - FINALIZE_PROMILLE);
                // This takes around 3 sec on a Atom Netbook
                for(auto an : as_const(m_aq)) {
                    an->finalize(nextTrack);
                }
                emit(trackDone(nextTrack));
                emitUpdateProgress(nextTrack, 1000); // 100%
            }
        } else {
            emitUpdateProgress(nextTrack, 1000); // 100%
            qDebug() << "Skipping track analysis because no analyzer initialized.";
        }
        emptyCheck();
    }
    emit(queueEmpty()); // emit in case of exit;
}

void AnalyzerQueue::emptyCheck()
{
    {
        QMutexLocker locker(&m_qm);
        m_queue_size = m_tioq.size();
    }
    if (m_queue_size == 0)
        emit(queueEmpty()); // emit asynchrony for no deadlock
}

// This is called from the AnalyzerQueue thread
void AnalyzerQueue::emitUpdateProgress(TrackPointer track, int progress)
{
    if (!m_exit) {
        // First tryAcqire will have always success because sema is initialized with on
        // The following tries will success if the previous signal was processed in the GUI Thread
        // This prevent the AnalysisQueue from filling up the GUI Thread event Queue
        // 100 % is emitted in any case
        if (progress < 1000 - FINALIZE_PROMILLE && progress > 0) {
            // Signals during processing are not required in any case
            if (!m_progressInfo.sema.tryAcquire()) {
               return;
            }
        } else {
            m_progressInfo.sema.acquire();
        }
        m_progressInfo.current_track = track;
        m_progressInfo.track_progress = progress;
        m_progressInfo.queue_size = m_queue_size;
        emit(updateProgress());
    }
}

//slot
void AnalyzerQueue::slotUpdateProgress()
{
    if (m_progressInfo.current_track) {
        m_progressInfo.current_track->setAnalyzerProgress(m_progressInfo.track_progress);
        m_progressInfo.current_track.reset();
    }
    emit(trackProgress(m_progressInfo.track_progress / 10));
    if (m_progressInfo.track_progress == 1000) {
        emit(trackFinished(m_progressInfo.queue_size));
    }
    m_progressInfo.sema.release();
}

void AnalyzerQueue::slotAnalyseTrack(TrackPointer tio)
{
    // This slot is called from the decks and and samplers when the track was loaded.
    queueAnalyseTrack(tio);
    m_aiCheckPriorities.store( true);
}

// This is called from the GUI and from the AnalyzerQueue thread
void AnalyzerQueue::queueAnalyseTrack(TrackPointer tio)
{
    QMutexLocker locker(&m_qm);
    if (!m_tioq.contains(tio)) {
        m_tioq.enqueue(tio);
        m_qwait.wakeAll();
    }
}

// static
AnalyzerQueue* AnalyzerQueue::createDefaultAnalyzerQueue(
        UserSettingsPointer pConfig, TrackCollection* pTrackCollection)
{
    auto ret = std::make_unique<AnalyzerQueue>(pTrackCollection);

    ret->addAnalyzer(new AnalyzerWaveform(pConfig));
    ret->addAnalyzer(new AnalyzerGain(pConfig));
    ret->addAnalyzer(new AnalyzerEbur128(pConfig));
#ifdef __VAMP__
    VampAnalyzer::initializePluginPaths();
    ret->addAnalyzer(new AnalyzerBeats(pConfig));
    ret->addAnalyzer(new AnalyzerKey(pConfig));
#endif

    ret->start(QThread::LowPriority);
    return ret.release();
}

// static
AnalyzerQueue* AnalyzerQueue::createAnalysisFeatureAnalyzerQueue(
        UserSettingsPointer pConfig, TrackCollection* pTrackCollection)
{
    auto ret = std::make_unique<AnalyzerQueue>(pTrackCollection);

    if (pConfig->getValue<bool>(ConfigKey("[Library]", "EnableWaveformGenerationWithAnalysis"))) {
        ret->addAnalyzer(new AnalyzerWaveform(pConfig));
    }
    ret->addAnalyzer(new AnalyzerGain(pConfig));
    ret->addAnalyzer(new AnalyzerEbur128(pConfig));
#ifdef __VAMP__
    VampAnalyzer::initializePluginPaths();
    ret->addAnalyzer(new AnalyzerBeats(pConfig));
    ret->addAnalyzer(new AnalyzerKey(pConfig));
#endif
    ret->start(QThread::LowPriority);
    return ret.release();
}
