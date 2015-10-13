// engineworker.cpp
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

#include "engine/engineworker.h"

EngineWorker::EngineWorker() = default;
EngineWorker::~EngineWorker() = default;
void EngineWorker::run() {}
void EngineWorker::wake()
{
  m_cond.notify_all();
}
void EngineWorker::pause()
{
  std::unique_lock<std::mutex> locker(m_mtx);
  m_cond.wait(locker);
}
