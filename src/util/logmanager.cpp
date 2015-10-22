#include "util/logmanager.h"
#include "util/cmdlineargs.h"
#include <QIODevice>
/* Debug message handler which outputs to both a logfile and a
 * and prepends the thread the message came from too.
 */
const int kLogPipeSize   = ( 1 << 10 );
const int kProcessLength = ( 1 <<  9 );

LogManager::LogManager()
  :QThread()
{
    setObjectName("LogManager");
    std::unique_lock<std::mutex> locker(m_log_file_lock);
    if(!m_log_file.isOpen())
    {
        // This Must be done in the Message Handler itself, to guarantee that the
        // QApplication is initialized
        auto logLocation = CmdlineArgs::Instance().getSettingsPath();
        auto logFileName = QString{};
        // Rotate old logfiles
        //FIXME: cerr << doesn't get printed until after mixxx quits (???)
        for (auto i = 9; i >= 0; --i)
        {
            if (i == 0) logFileName = QDir(logLocation).filePath("mixxx.log");
            else        logFileName = QDir(logLocation).filePath(QString("mixxx.log.%1").arg(i));
            auto logbackup = QFileInfo(logFileName);
            if (logbackup.exists())
            {
                auto olderlogname = QDir(logLocation).filePath(QString("mixxx.log.%1").arg(i + 1));
                // This should only happen with number 10
                if (QFileInfo(olderlogname).exists()) QFile::remove(olderlogname);
                if (!QFile::rename(logFileName, olderlogname)) std::cerr << "Error rolling over logfile " << logFileName.toStdString();
            }
        }
        // WARNING(XXX) getSettingsPath() may not be ready yet. This causes
        // Logfile writes below to print qWarnings which in turn recurse into
        // MessageHandler -- potentially deadlocking.
        // XXX will there ever be a case that we can't write to our current
        // working directory?
        m_log_file.setFileName(logFileName);
        m_log_file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    m_msg_types << "[ %1 ] Debug: %2\n" << "[ %1 ] Warning %2\n" << "[ %1 ] Critical: %2\n" << "[ %1 ] Fatal: %2\n" << "[ %1 ] Info: %2\n";
    start(QThread::HighPriority);
}
LogManager::~LogManager()
{
  m_quit.store(1);
  notify();
  wait();
  {
    std::unique_lock<std::mutex> locker(m_log_lock);
    process();
  }
  qDebug() << "Mixxx logger shutting down.";
  qInstallMessageHandler(nullptr);
  {
    std::unique_lock<std::mutex> locker(m_log_file_lock);
    if ( m_log_file.isOpen() )
    {
      m_log_file.flush();
      m_log_file.close();
    }
  }
}
LogPipe::LogPipe(LogManager *pManager)
  :FIFO<QByteArray>(kLogPipeSize)
  ,m_pManager(pManager)
  ,m_threadName(QThread::currentThread()->objectName())
{
  qRegisterMetaType<QByteArray>("QByteArray");
}
LogPipe::~LogPipe()
{
  if ( m_pManager) m_pManager->onLogPipeDestroyed(this);
}
void LogPipe::put ( QtMsgType t, const QString &msg )
{
  put ( m_pManager->format_msg (t, m_threadName, msg ) );

  m_pManager->notify();
}
void LogManager::onLogPipeDestroyed(LogPipe* pPipe)
{
  std::unique_lock<std::mutex> locker(m_log_lock);
  process();
  m_log_pipes.removeAll(pPipe);
}
LogPipe* LogManager::get_pipe()
{
  if ( m_thread_pipes.hasLocalData()) return m_thread_pipes.localData();
  auto pResult = new LogPipe(this);
  m_thread_pipes.setLocalData(pResult);
  std::unique_lock<std::mutex> locker(m_log_lock);
  m_log_pipes.push_back(pResult);
  return pResult;
}
void LogManager::process()
{
  for ( auto pLogPipe : m_log_pipes)
  {
    if ( pLogPipe)
    {
      auto message = QByteArray{};
      while ( pLogPipe->pop ( message ) )
      {
        std::cerr << message.constData() ;
        if(m_log_file.isOpen())
        {
          m_log_file.write(message);
          m_log_file.flush();
        }
      }
    }
  }
}
void LogManager::run()
{
    std::unique_lock<std::mutex>  locker(m_log_lock);
    while(!m_quit.load())
    {
        process();
        m_log_cond.wait_for ( locker, std::chrono::milliseconds(500) );
    }
}
/* static*/ void LogManager::messageHandler(QtMsgType type,const QMessageLogContext&, const QString& input)
{
    if ( auto logManager = LogManager::instance() )
    {
      if ( auto logPipe = logManager->get_pipe() )
      {
        logPipe->put ( type, input );
      }
    }
}
QByteArray LogManager::format_msg ( QtMsgType t, const QString& thrd, const QString& msg ) const
{
  auto i = static_cast<int>(t);
  if ( i < 0 || i > m_msg_types.size() ) return QByteArray{};
  return m_msg_types.at ( i ) .arg ( thrd, msg ).toLocal8Bit();
}
void LogManager::notify()
{
  m_log_cond.notify_all();
}
