_Pragma("once")
#include <QMap>
#include <QObject>
#include <QString>
#include <QThread>
#include <atomic>
#include <QtDebug>
#include <QMutex>
#include <QWaitCondition>
#include <QSemaphore>
#include <QThreadStorage>
#include <QSharedPointer>
#include <QLinkedList>
#include <QList>

#include "util/fifo.h"
#include "util/singleton.h"
#include "util/stat.h"
#include "util/event.h"

class StatsManager;

class StatsPipe : public FIFO<StatReport> {
  public:
    StatsPipe(StatsManager* pManager);
    virtual ~StatsPipe();
  private:
    StatsManager* m_pManager;
};
class StatsManager : public QThread, public Singleton<StatsManager> {
    Q_OBJECT
  public:
    explicit StatsManager();
    virtual ~StatsManager();
    // Returns true if write succeeds.
    bool maybeWriteReport(const StatReport& report);
    static bool s_bStatsManagerEnabled;
    // Tell the StatsManager to emit statUpdated for every stat that exists.
    void emitAllStats() {m_emitAllStats = 1;}
    void updateStats() {m_statsSem.release();}
  signals:
    void statUpdated(const Stat& stat);
  protected:
    virtual void run();
  private:
    void processIncomingStatReports();
    QSharedPointer<StatsPipe> getStatsPipeForThread();
    void onStatsPipeDestroyed(StatsPipe* pPipe);
    void writeTimeline(const QString& filename);
    std::atomic<bool>                          m_emitAllStats;
    std::atomic<bool>                          m_quit;
    QMap<QString, Stat>                        m_stats;
    QMap<QString, Stat>                        m_baseStats;
    QMap<QString, Stat>                        m_experimentStats;
    QList<Event>                               m_events;
    QSemaphore                                 m_statsSem;
    QMutex m_statsPipeLock;
    QLinkedList<QWeakPointer<StatsPipe > >     m_statsPipes;
    QThreadStorage<QSharedPointer<StatsPipe> > m_threadStatsPipes;
    friend class StatsPipe;
};
