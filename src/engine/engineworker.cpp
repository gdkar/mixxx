// engineworker.cpp
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

#include "engine/engineworker.h"
#include "engine/engineworkerscheduler.h"

EngineWorker::EngineWorker()
    : m_pScheduler(nullptr) {
}
EngineWorker::~EngineWorker() = default;
void EngineWorker::run()
{
}
void EngineWorker::setScheduler(EngineWorkerScheduler* pScheduler)
{
    m_pScheduler = pScheduler;
}
bool EngineWorker::workReady()
{
    wake();
    return true;
}
