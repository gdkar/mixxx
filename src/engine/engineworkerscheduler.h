#ifndef ENGINEWORKERSCHEDULER_H
#define ENGINEWORKERSCHEDULER_H

#include <QThreadPool>
#include <QThread>

#include <atomic>
#include "util/fifo.h"
#include "util/semaphore.hpp"
// The max engine workers that can be expected to run within a callback
// (e.g. the max that we will schedule). Must be a power of 2.

class EngineWorker;

class EngineWorkerScheduler {
    static constexpr const size_t MAX_ENGINE_WORKERS = 32;
  // Indicates whether workerReady has been called since the last time
    // runWorkers was run. This should only be touched from the engine callback.
    FIFO<EngineWorker*> m_scheduleFIFO{MAX_ENGINE_WORKERS};
    std::atomic<bool> m_bRunWorkers{false};
  public:
    EngineWorkerScheduler();
   ~EngineWorkerScheduler();
    void runWorkers();
    void workerReady(EngineWorker* worker);
};
#endif /* ENGINEWORKERSCHEDULER_H */
