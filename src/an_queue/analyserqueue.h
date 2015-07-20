#ifndef ANALYSERQUEUE_H
#define ANALYSERQUEUE_H

#include "configobject.h"
#include "an_queue/analyser.h"
#include "trackinfoobject.h"
#include "sources/audiosource.h"
#include "samplebuffer.h"

#include <QList>
#include <QThread>
#include <QQueue>
#include <QWaitCondition>
#include <QSemaphore>

#include <vector>
#include <atomic>

class TrackCollection;

class AnalyserQueue : public QThread {
    Q_OBJECT
  public:
    explicit AnalyserQueue(TrackCollection* pTrackCollection);
    virtual ~AnalyserQueue();
    virtual void stop();
    virtual void queueAnalyseTrack(TrackPointer tio);
    static AnalyserQueue* createDefaultAnalyserQueue(
            ConfigObject<ConfigValue>* pConfig, TrackCollection* pTrackCollection);
    static AnalyserQueue* createAnalysisFeatureAnalyserQueue(
            ConfigObject<ConfigValue>* pConfig, TrackCollection* pTrackCollection);
  public slots:
    virtual void slotAnalyseTrack(TrackPointer tio);
    virtual void slotUpdateProgress();
  signals:
    void trackProgress(double progress);
    void trackDone(TrackPointer track);
    void trackFinished(int size);
    // Signals from AnalyserQueue Thread:
    void queueEmpty();
    void updateProgress();
  protected:
    void run();
  private:
    struct progress_info {
        TrackPointer current_track;
        std::atomic<double> track_progress; // in 0.1 %
        std::atomic<int> queue_size;
        QSemaphore sema;
    };
    void addAnalyser(Analyser* an);
    QList<Analyser*> m_aq;
    bool isLoadedTrackWaiting(TrackPointer tio);
    TrackPointer dequeueNextBlocking();
    bool doAnalysis(TrackPointer tio, Mixxx::AudioSourcePointer pAudioSource);
    void emitUpdateProgress(TrackPointer tio, double progress);
    bool m_exit;
    std::atomic<bool> m_aiCheckPriorities;
    SampleBuffer m_sampleBuffer;
    // The processing queue and associated mutex
    QQueue<TrackPointer> m_tioq;
    QMutex m_qm;
    QWaitCondition m_qwait;
    struct progress_info m_progressInfo;
    int m_queue_size;
};

#endif
