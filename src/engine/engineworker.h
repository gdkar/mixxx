// engineworker.h
// Created 6/2/2010 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <atomic>
#include <QObject>
#include <condition_variable>
#include <mutex>
#include <QThread>

class EngineWorker : public QThread {
    Q_OBJECT
  public:
    EngineWorker();
    virtual ~EngineWorker();
    virtual void run();
    void wake();
  protected:
    void pause();
    std::mutex              m_mtx;
    std::condition_variable m_cond;
};
