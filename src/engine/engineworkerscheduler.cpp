// engineworkerscheduler.cpp
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>

#include "engine/engineworker.h"
#include "engine/engineworkerscheduler.h"
#include "util/event.h"

EngineWorkerScheduler::EngineWorkerScheduler(QObject* pParent)
        : m_bWakeScheduler(false),
          m_scheduleFIFO(MAX_ENGINE_WORKERS),
          m_bQuit(false)
{
    Q_UNUSED(pParent);
}

EngineWorkerScheduler::~EngineWorkerScheduler()
{
    m_bQuit = true;
    m_waitSema.release();
    wait();
}

void EngineWorkerScheduler::workerReady(EngineWorker* pWorker)
{
    if (pWorker) {
        // If the write fails, we really can't do much since we should not block
        // in this slot. Write the address of the variable pWorker, since it is
        // a 1-element array.
        m_scheduleFIFO.push_back(pWorker);
        m_bWakeScheduler = true;
    }
}

void EngineWorkerScheduler::runWorkers()
{
    // Wake the scheduler if we have written a worker-ready message to the
    // scheduler. There is no race condition in accessing this boolean because
    // both workerReady and runWorkers are called from the callback thread.
    if (m_bWakeScheduler.exchange(false))
    {
        m_waitSema.release();
    }
}

void EngineWorkerScheduler::run()
{
    while (!m_bQuit.load()) {
        Event::start("EngineWorkerScheduler");
        while (!m_scheduleFIFO.empty()){
            if(auto pWorker = m_scheduleFIFO.front()) {
                pWorker->wake();
            }
            m_scheduleFIFO.pop_front();
        }
        Event::end("EngineWorkerScheduler");
        if (!m_bQuit.load())
            m_waitSema.acquire();
    }
}
