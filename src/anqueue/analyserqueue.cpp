#include <typeinfo>

#include <QtDebug>
#include <QMutexLocker>
#include <atomic>
#include "trackinfoobject.h"
#include "playerinfo.h"
#include "analyserqueue.h"
#include "soundsourceproxy.h"
#include "playerinfo.h"
#include "util/timer.h"
#include "library/trackcollection.h"
#include "analyserwaveform.h"
#include "analyserrg.h"
#include "analyserbeats.h"
#include "analyserkey.h"
#include "vamp/vampanalyser.h"
#include "util/compatibility.h"
#include "util/event.h"
#include "util/trace.h"

// Measured in 0.1%
// 0 for no progress during finalize
// 1 to display the text "finalizing"
// 100 for 10% step after finalize
namespace {
    // Analysis is done in blocks.
    // We need to use a smaller block size, because on Linux the AnalyserQueue
    // can starve the CPU of its resources, resulting in xruns. A block size
    // of 4096 frames per block seems to do fine.
    const SINT kAnalysisChannels        = Mixxx::AudioSource::kChannelCountStereo;
    const SINT kAnalysisFrameRate       = 0;
    const SINT kAnalysisFramesPerBlock  = 4096;
    const SINT kAnalysisSamplesPerBlock = kAnalysisFramesPerBlock * kAnalysisChannels;
} // anonymous namespace
AnalyserQueue::AnalyserQueue(TrackCollection* /*pTrackCollection*/,QObject *p)
        : QThread(p),
          m_sampleBuffer(kAnalysisSamplesPerBlock)
{
    connect(this, SIGNAL(updateProgress()), this, SLOT(slotUpdateProgress()));
}
AnalyserQueue::~AnalyserQueue() {
    stop();
    m_progressInfo.sema.release();
    wait(); //Wait until thread has actually stopped before proceeding.
    m_aq.clear();
    //qDebug() << "AnalyserQueue::~AnalyserQueue()";
}
void AnalyserQueue::addAnalyser(Analyser* an)
{
  m_aq.push_back(an);
}
// This is called from the AnalyserQueue thread
bool AnalyserQueue::isLoadedTrackWaiting(TrackPointer analysingTrack)
{
    auto &info = PlayerInfo::instance();
    auto pTrack = TrackPointer{nullptr};
    auto trackWaiting = false;
    QList<TrackPointer> progress100List;
    QList<TrackPointer> progress0List;
    m_qm.lock();
    QMutableListIterator<TrackPointer> it(m_tioq);
    while (it.hasNext()) {
        TrackPointer& pTrack = it.next();
        if (!pTrack) {
            it.remove();
            continue;
        }
        if (!trackWaiting) { trackWaiting = info.isTrackLoaded(pTrack); }
        // try to load waveforms for all new tracks first
        // and remove them from queue if already analysed
        // This avoids waiting for a running analysis for those tracks.
        auto progress = pTrack->getAnalyserProgress();
        if (progress < 0) {
            // Load stored analysis
            auto processTrack = false;
            for ( auto ita : m_aq ) if ( !ita->loadStored(pTrack) ){processTrack = true;}
            if (!processTrack) {
                it.remove();
                progress100List.append(pTrack);
            } else { progress0List.append(pTrack);}
        } else if (progress == 1.0) { it.remove(); }
    }
    m_qm.unlock();
    // update progress after unlock to avoid a deadlock
    for(auto pTrack: progress0List)     emitUpdateProgress(pTrack, 0);
    for(auto pTrack: progress100List)   emitUpdateProgress(pTrack, 1.0);
    if (info.isTrackLoaded(analysingTrack)) 
      return false;
    return trackWaiting;
}
// This is called from the AnalyserQueue thread
TrackPointer AnalyserQueue::dequeueNextBlocking() {
    m_qm.lock();
    if (m_tioq.isEmpty()) {
        Event::end("AnalyserQueue process");
        m_qwait.wait(&m_qm);
        Event::start("AnalyserQueue process");
        if (m_abExit.load()) {
            m_qm.unlock();
            return TrackPointer();
        }
    }
    const auto& info = PlayerInfo::instance();
    auto pLoadTrack = TrackPointer{nullptr};
    auto new_list = decltype(m_tioq){};
    for ( auto pTrack : m_tioq )
    {
      if ( !pTrack ) continue;
      if ( !pLoadTrack && info.isTrackLoaded ( pTrack ) )
      {
        qDebug() << "Prioritizing" << pTrack->getTitle() << pTrack->getLocation();
        pLoadTrack = pTrack;
        continue;
      }
      new_list.append(pTrack);
    }
    m_tioq.swap ( new_list );
    // no prioritized track found, use head track
    if (!pLoadTrack && !m_tioq.isEmpty()) { pLoadTrack = m_tioq.dequeue(); }
    m_qm.unlock();
    if (pLoadTrack) { qDebug() << "Analyzing" << pLoadTrack->getTitle() << pLoadTrack->getLocation(); }
    // pTrack might be NULL, up to the caller to check.
    return pLoadTrack;
}

// This is called from the AnalyserQueue thread
bool AnalyserQueue::doAnalysis(TrackPointer tio, Mixxx::AudioSourcePointer pAudioSource) {
    QTime progressUpdateInhibitTimer;
    progressUpdateInhibitTimer.start(); // Inhibit Updates for 60 milliseconds
    auto frameIndex = pAudioSource->getMinFrameIndex();
    auto dieflag = false;
    auto cancelled = false;
    do {
        ScopedTimer t("AnalyserQueue::doAnalysis block");
        if ( frameIndex >= pAudioSource->getMaxFrameIndex() )
        {
          qDebug()
            << __FUNCTION__
            << " invalid starting frame index "
            << frameIndex
            << " should be 0 <= frameIndex < "
            << pAudioSource->getMaxFrameIndex();
        }
        auto framesRemaining = pAudioSource->getMaxFrameIndex() - frameIndex;
        auto framesToRead    = math_min(kAnalysisFramesPerBlock, framesRemaining);
        auto framesRead      = pAudioSource->readSampleFramesStereo( framesToRead, &m_sampleBuffer);
        if ( framesRead > framesToRead || framesRead < 0 )
        {
          qDebug() 
            << __FUNCTION__ 
            << " invalid read amount. requested " 
            << framesToRead 
            << " and got " 
            << framesRead;
        }
        frameIndex += framesRead;
        if ( frameIndex < 0 || frameIndex > pAudioSource->getMaxFrameIndex ( ) )
        {
          qDebug() 
            << __FUNCTION__ 
            << " invalid frame index " 
            << frameIndex 
            << " should be 0 <= frameIndex < " 
            << pAudioSource->getMaxFrameIndex ( );
        }
        // To compare apples to apples, let's only look at blocks that are
        // the full block size.
        auto frameProgress = 0.0;
        if (kAnalysisFramesPerBlock == framesRead) {
            // Complete analysis block of audio samples has been read.
            for(auto an : m_aq )
            {
              an->process(m_sampleBuffer.data(), m_sampleBuffer.size());
            }
            frameProgress = std::min<double>(double(frameIndex) / double(pAudioSource->getMaxFrameIndex()),1.0);
        } else {
            // Partial analysis block of audio samples has been read.
            // This should only happen at the end of an audio stream,
            // otherwise a decoding error must have occurred.
            frameProgress = 1.0;
            if (frameIndex < pAudioSource->getMaxFrameIndex()) {
                // EOF not reached -> Maybe a corrupt file?
                qWarning() << "Failed to read sample data from file:" << tio->getFilename() << "@" << frameIndex;
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
        // because the finalise functions will take also some time
        //fp div here prevents insane signed overflow
        if (m_progressInfo.track_progress != frameProgress) {
            tio->setAnalyserProgress(frameProgress);
            emitUpdateProgress(tio, frameProgress);
        }
        // Since this is a background analysis queue, we should co-operatively
        // yield every now and then to try and reduce CPU contention. The
        // analyser queue is CPU intensive so we want to get out of the way of
        // the audio callback thread.
        //QThread::yieldCurrentThread();
        //QThread::usleep(10);
        // has something new entered the queue?
        if (m_abCheckPriorities.exchange(false)) {
            if (isLoadedTrackWaiting(tio)) {
                qDebug() << "Interrupting analysis to give preference to a loaded track.";
                dieflag = true;
                cancelled = true;
            }
        }
        if (m_abExit.load()) {
            dieflag = true;
            cancelled = true;
        }
        // Ignore blocks in which we decided to bail for stats purposes.
        if (dieflag || cancelled) 
          t.cancel();
    } while (!dieflag && (frameIndex < pAudioSource->getMaxFrameIndex()));
    return !cancelled; //don't return !dieflag or we might reanalyze over and over
}
void AnalyserQueue::stop()
{
    m_abExit.store(true);
    m_qm.lock();
    m_qwait.wakeAll();
    m_qm.unlock();
}
void AnalyserQueue::run()
{
    static std::atomic<int> id{0}; //the id of this thread, for debugging purposes
    QThread::currentThread()->setObjectName(QString("AnalyserQueue %1").arg(++id));
    // If there are no analyzers, don't waste time running.
    if (!m_aq.size()) return;
    m_progressInfo.current_track.clear();
    m_progressInfo.track_progress = 0;
    m_progressInfo.queue_size = 0;
    m_progressInfo.sema.release(); // Initialise with one
    while (!m_abExit.load()) 
    {
        auto nextTrack = dequeueNextBlocking();
        // It's important to check for m_exit here in case we decided to exit
        // while blocking for a new track.
        if (m_abExit.load()) break;
        // If the track is NULL, try to get the next one.
        // Could happen if the track was queued but then deleted.
        // Or if dequeueNextBlocking is unblocked by exit == true
        if (!nextTrack)
        {
            emptyCheck();
            continue;
        }
        Trace trace("AnalyserQueue analyzing track");
        // Get the audio
        auto soundSourceProxy= SoundSourceProxy{nextTrack};
        auto audioSrcCfg = Mixxx::AudioSourceConfig{ kAnalysisChannels, kAnalysisFrameRate};
        auto pAudioSource = soundSourceProxy.openAudioSource(audioSrcCfg);
        if (!pAudioSource) {
            qWarning() << "Failed to open file for analyzing:" << nextTrack->getLocation();
            emptyCheck();
            continue;
        }
        auto processTrack = false;
        for ( auto it : m_aq )
        {
          if ( it  &&  it->initialise( nextTrack,  pAudioSource->getFrameRate(), pAudioSource->getFrameCount() * kAnalysisChannels ))
            processTrack = true;
        }
        m_qm.lock();
        m_queue_size = m_tioq.size();
        m_qm.unlock();
        if (processTrack) {
            emitUpdateProgress(nextTrack, 0);
            nextTrack->setAnalyserProgress(0);
            auto completed = doAnalysis(nextTrack, pAudioSource);
            if (!completed) {
                // This track was cancelled
                for(auto &itf : m_aq ) itf->cleanup(nextTrack);
                queueAnalyseTrack(nextTrack);
                emitUpdateProgress(nextTrack, 0);
                nextTrack->setAnalyserProgress(0);
            } else {
                // 100% - FINALIZE_PERCENT finished
                nextTrack->setAnalyserProgress(1.0 );
                // This takes around 3 sec on a Atom Netbook
                for(auto itf : m_aq ) itf->finalise(nextTrack);
                nextTrack->setAnalyserProgress(1.0);
            }
        } else {
            emitUpdateProgress(nextTrack, 1.0); // 100%
            nextTrack->setAnalyserProgress(1.0);
            qDebug() << "Skipping track analysis because no analyzer initialized.";
        }
        emptyCheck();
    }
    emit(queueEmpty()); // emit in case of exit;
}
void AnalyserQueue::emptyCheck()
{
    m_qm.lock();
    m_queue_size.store(m_tioq.size());
    m_qm.unlock();
    if (!m_queue_size.load()) emit(queueEmpty());
}
// This is called from the AnalyserQueue thread
void AnalyserQueue::emitUpdateProgress(TrackPointer tio, double progress)
{
    if (!m_abExit.load()) {
        // First tryAcqire will have always success because sema is initialized with on
        // The following tries will success if the previous signal was processed in the GUI Thread
        // This prevent the AnalysisQueue from filling up the GUI Thread event Queue
        // 100 % is emitted in any case
        m_progressInfo.current_track  = tio;
        m_progressInfo.track_progress = progress;
        m_progressInfo.queue_size     = m_queue_size;
        emit(updateProgress());
    }
}
//slot
void AnalyserQueue::slotUpdateProgress() {
    auto prog = m_progressInfo.track_progress;
    emit(trackProgress(prog));
    if (auto ct = m_progressInfo.current_track) {
        m_progressInfo.current_track.clear();
    }
    if ( prog == 1) { emit(trackFinished(m_progressInfo.queue_size)); }
    m_progressInfo.sema.release();
}
void AnalyserQueue::slotAnalyseTrack(TrackPointer tio) {
    // This slot is called from the decks and and samplers when the track was loaded.
    queueAnalyseTrack(tio);
    m_abCheckPriorities.store(true);
}
// This is called from the GUI and from the AnalyserQueue thread
void AnalyserQueue::queueAnalyseTrack(TrackPointer tio) {
    m_qm.lock();
    if (!m_tioq.contains(tio)) {
        m_tioq.enqueue(tio);
        m_qwait.wakeAll();
    }
    m_qm.unlock();
}
// static
AnalyserQueue* AnalyserQueue::createDefaultAnalyserQueue( ConfigObject<ConfigValue>* pConfig, TrackCollection* pTrackCollection, QObject *pParent)
{
    VampAnalyser::initializePluginPaths();
    auto ret = new AnalyserQueue(pTrackCollection, pParent);
    ret->addAnalyser(new AnalyserWaveform(pConfig,ret));
    ret->addAnalyser(new AnalyserGain(pConfig,ret));
    ret->addAnalyser(new AnalyserBeats(pConfig,ret));
    ret->addAnalyser(new AnalyserKey(pConfig,ret));
    ret->start(QThread::LowPriority);
    return ret;
}
// static
AnalyserQueue* AnalyserQueue::createAnalysisFeatureAnalyserQueue( ConfigObject<ConfigValue>* pConfig, TrackCollection* pTrackCollection,QObject *pParent)
{
    VampAnalyser::initializePluginPaths();
    auto ret = new AnalyserQueue(pTrackCollection,pParent);
    ret->addAnalyser(new AnalyserGain(pConfig,ret));
    ret->addAnalyser(new AnalyserBeats(pConfig,ret));
    ret->addAnalyser(new AnalyserKey(pConfig,ret));
    ret->start(QThread::LowPriority);
    return ret;
}
