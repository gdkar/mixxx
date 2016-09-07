// engineworker.h
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

#ifndef ENGINEWORKER_H
#define ENGINEWORKER_H

#include <QAtomicInt>
#include <QObject>
#include "util/semaphore.hpp"
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
    void wake() { m_semaRun.release(); }
  protected:
    mixxx::MSemaphore m_semaRun;

  private:
    EngineWorkerScheduler* m_pScheduler;
};

#endif /* ENGINEWORKER_H */
