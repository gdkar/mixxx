// engineworker.h
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <QObject>
#include <QThread>

// EngineWorker is an interface for running background processing work when the
// audio callback is not active. While the audio callback is active, an
// EngineWorker can emit its workReady signal, and an EngineWorkerManager will
// schedule it for running after the audio callback has completed.

class EngineWorkerScheduler;

class EngineWorker : public QThread {
    Q_OBJECT
  public:
    EngineWorker();
    virtual ~EngineWorker();
    virtual void run();
    void setScheduler(EngineWorkerScheduler* pScheduler);
    bool workReady();
    void wake() {
        m_condv.notify_all();
    }
  protected:
    std::condition_variable m_condv;
    std::mutex              m_mutex;
  private:
    EngineWorkerScheduler* m_pScheduler;
};
