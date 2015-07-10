// engineworker.cpp
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

#include "engine/engineworker.h"
#include "engine/engineworkerscheduler.h"

EngineWorker::EngineWorker(QObject*pParent)
    : QThread(pParent),
    m_pScheduler(nullptr) {

}

EngineWorker::~EngineWorker() {
}

void EngineWorker::run() {
}

void EngineWorker::setScheduler(EngineWorkerScheduler* pScheduler) {
    m_pScheduler = pScheduler;
}

bool EngineWorker::workReady() {
    if (m_pScheduler) {
        m_pScheduler->workerReady(this);
        return true;
    }
    return false;
}
