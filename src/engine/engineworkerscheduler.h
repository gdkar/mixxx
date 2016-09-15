#ifndef ENGINEWORKERSCHEDULER_H
#define ENGINEWORKERSCHEDULER_H

#include <QMutex>
#include <QThreadPool>
#include <QWaitCondition>

#include "util/semaphore.hpp"
#include "util/fifo.h"

// The max engine workers that can be expected to run within a callback
// (e.g. the max that we will schedule). Must be a power of 2.
#define MAX_ENGINE_WORKERS 32

class EngineWorker;

class EngineWorkerScheduler : public QThread {
    Q_OBJECT
  public:
    EngineWorkerScheduler(QObject* pParent=NULL);
    virtual ~EngineWorkerScheduler();

    void runWorkers();
    void workerReady(EngineWorker* worker);

  protected:
    void run();

  private:
    // Indicates whether workerReady has been called since the last time
    // runWorkers was run. This should only be touched from the engine callback.
    std::atomic<bool> m_bWakeScheduler{false};

    FIFO<EngineWorker*> m_scheduleFIFO;
    mixxx::MSemaphore m_waitSema{};
    std::atomic<bool> m_bQuit{false};
};

#endif /* ENGINEWORKERSCHEDULER_H */
