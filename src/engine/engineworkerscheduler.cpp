// engineworkerscheduler.cpp
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>

#include "engine/engineworker.h"
#include "engine/engineworkerscheduler.h"
#include "util/event.h"

EngineWorkerScheduler::EngineWorkerScheduler(QObject* pParent)
        : m_bWakeScheduler(false),
          m_scheduleFIFO(),
          m_waitSem(1),
          m_bQuit(false) {
    Q_UNUSED(pParent);
}

EngineWorkerScheduler::~EngineWorkerScheduler() {
    m_bQuit = true;
    m_waitSem.release();
    wait();
}

void EngineWorkerScheduler::workerReady(EngineWorker* pWorker) {
    if (pWorker) {
        // If the write fails, we really can't do much since we should not block
        // in this slot. Write the address of the variable pWorker, since it is
        // a 1-element array.
        m_scheduleFIFO.write(pWorker);
        m_bWakeScheduler = true;
    }
}

void EngineWorkerScheduler::runWorkers() {
    // Wake the scheduler if we have written a worker-ready message to the
    // scheduler. There is no race condition in accessing this boolean because
    // both workerReady and runWorkers are called from the callback thread.
    if (m_bWakeScheduler) {m_bWakeScheduler = false;m_waitSem.release();}
}

void EngineWorkerScheduler::run() {
    while (!m_bQuit) {
        Event::start("EngineWorkerScheduler");
        EngineWorker* pWorker = NULL;
        while (m_scheduleFIFO.read(pWorker) ) {if (pWorker) {pWorker->wake();}}
        Event::end("EngineWorkerScheduler");
//        m_mutex.lock();
        m_waitSem.acquire();
//        m_mutex.unlock();
    }
}
