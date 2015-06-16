// engineworkerscheduler.cpp
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>

#include "engine/engineworker.h"
#include "engine/engineworkerscheduler.h"
#include "util/event.h"

EngineWorkerScheduler::EngineWorkerScheduler(QObject* pParent)
        : QThread(pParent)
        , m_scheduleFIFO(MAX_ENGINE_WORKERS){
    Q_UNUSED(pParent);
}

EngineWorkerScheduler::~EngineWorkerScheduler() {
    m_bQuit.store(true);
    m_waitSemaphore.release();
    wait();
}

void EngineWorkerScheduler::workerReady(EngineWorker* pWorker) {
    if (pWorker) {
        // If the write fails, we really can't do much since we should not block
        // in this slot. Write the address of the variable pWorker, since it is
        // a 1-element array.
        m_scheduleFIFO.write(&pWorker, 1);
        m_bWakeScheduler.store(true);
    }
}

void EngineWorkerScheduler::runWorkers() {
    // Wake the scheduler if we have written a worker-ready message to the
    // scheduler. There is no race condition in accessing this boolean because
    // both workerReady and runWorkers are called from the callback thread.
    if (m_bWakeScheduler.exchange(false)) {m_waitSemaphore.release();}
}

void EngineWorkerScheduler::run() {
    while (!m_bQuit.load()) {
        Event::start("EngineWorkerScheduler");
        EngineWorker* pWorker = NULL;
        while (m_scheduleFIFO.read(&pWorker, 1) == 1) {if (pWorker) {pWorker->run();delete pWorker;}}
        Event::end("EngineWorkerScheduler");
        m_waitSemaphore.acquire(); // unlock mutex and wait
    }
}
