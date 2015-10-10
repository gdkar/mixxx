_Pragma("once")
#include "configobject.h"
#include "analyser.h"
#include "trackinfoobject.h"
#include "sources/audiosource.h"
#include "samplebuffer.h"

#include <QList>
#include <QThread>
#include <QQueue>
#include <QWaitCondition>
#include <QSemaphore>

#include <vector>
class TrackCollection;
class AnalyserQueue : public QThread {
    Q_OBJECT
  public:
    AnalyserQueue(TrackCollection* pTrackCollection,QObject *pParent);
    virtual ~AnalyserQueue();
    void stop();
    void queueAnalyseTrack(TrackPointer tio);
    static AnalyserQueue* createDefaultAnalyserQueue(ConfigObject<ConfigValue>* pConfig, TrackCollection* pTrackCollection, QObject *pParent);
    static AnalyserQueue* createAnalysisFeatureAnalyserQueue(ConfigObject<ConfigValue>* pConfig, TrackCollection* pTrackCollection, QObject *pParent);
    struct progress_info
    {
        TrackPointer current_track{nullptr};
        double track_progress{0}; 
        int queue_size = 0;
        QSemaphore sema{0};
    };
  public slots:
    void slotAnalyseTrack(TrackPointer tio);
    void slotUpdateProgress();
  signals:
    void trackProgress(double  progress);
    void trackFinished(int size);
    // Signals from AnalyserQueue Thread:
    void queueEmpty();
    void updateProgress();
  protected:
    void run();
  private:
    void addAnalyser(Analyser* an);
    QList<Analyser*> m_aq;
    bool isLoadedTrackWaiting(TrackPointer analysingTrack);
    TrackPointer dequeueNextBlocking();
    bool doAnalysis(TrackPointer tio, Mixxx::AudioSourcePointer pAudioSource);
    void emitUpdateProgress(TrackPointer tio, double progress);
    void emptyCheck();
    std::atomic<bool> m_abExit{false};
    std::atomic<bool> m_abCheckPriorities{false};
    SampleBuffer m_sampleBuffer;
    // The processing queue and associated mutex
    QQueue<TrackPointer> m_tioq;
    QMutex m_qm;
    QWaitCondition m_qwait;
    progress_info m_progressInfo;
    std::atomic<int> m_queue_size{0};
};

