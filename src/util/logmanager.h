_Pragma("once")

#include <QObject>
#include <QString>
#include <QThread>
#include <QDebug>
#include <QSemaphore>
#include <QThreadStorage>
#include <QApplication>
#include <QList>
#include <QFile>

#include <cstdio>
#include <iostream>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "util/fifo.h"
#include "util/singleton.h"

class LogManager;
class LogPipe : public FIFO<QByteArray> {
    LogManager *const m_pManager;
    QString     m_threadName = QString{"?"};
    using FIFO<QByteArray>::put;
  public:
    LogPipe(LogManager* pManager);
    virtual ~LogPipe();
    void put ( QtMsgType t, const QString& );
};
class LogManager : public QThread, public Singleton<LogManager>
{
  Q_OBJECT;
  std::atomic<bool>          m_quit{false};
  std::condition_variable    m_log_cond;
  std::mutex                 m_log_lock;
  QList<LogPipe*>            m_log_pipes;
  QThreadStorage<LogPipe*>   m_thread_pipes;
  QFile                      m_log_file;
  std::mutex                 m_log_file_lock;
  QStringList                m_msg_types;
public:
  explicit LogManager();
  virtual ~LogManager();
  static  void        messageHandler(QtMsgType type, const QMessageLogContext&ctx, const QString& msg);
  virtual void        run();
  virtual void        process();
  virtual LogPipe    *get_pipe();
  virtual void        onLogPipeDestroyed(LogPipe*);
  virtual void        notify();
  virtual QByteArray  format_msg ( QtMsgType, const QString & name, const QString& msg) const;
};
