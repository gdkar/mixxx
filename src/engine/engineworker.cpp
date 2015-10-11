// engineworker.cpp
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

#include "engine/engineworker.h"

EngineWorker::EngineWorker() = default;
EngineWorker::~EngineWorker() = default;
void EngineWorker::run()
{
}
void EngineWorker::wake()
{
  m_condv.notify_all();
}
