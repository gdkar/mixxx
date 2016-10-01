#ifndef STATSMANAGER_H
#define STATSMANAGER_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QThread>
#include <QAtomicInt>
#include <QtDebug>
#include <QMutex>
#include <QWaitCondition>
#include <QThreadStorage>
#include <QList>

#include "util/fifo.h"
#include "util/singleton.h"
#include "util/semaphore.hpp"
#include "util/stat.h"
#include "util/event.h"

/*class StatsManager;

class StatsPipe : public FIFO<StatReport> {
  public:
    StatsPipe(StatsManager* pManager);
    virtual ~StatsPipe();
  private:
    StatsManager* m_pManager;
};*/

class StatsManager : public QThread, public Singleton<StatsManager> {
    Q_OBJECT
  public:
    explicit StatsManager();
    virtual ~StatsManager();
    // Returns true if write succeeds.
    bool maybeWriteReport(std::unique_ptr<StatReport>& report);
    bool maybeWriteReport(StatReport* report);

    static bool s_bStatsManagerEnabled;
    // Tell the StatsManager to emit statUpdated for every stat that exists.
  public slots:
    void emitAllStats()
    {
        m_emitAllStats = 1;
    }
    void updateStats()
    {
        m_statsSema.release();
    }
    void resetStats()
    {
        m_resetStats.store(1);
        m_statsSema.release();
    }
  signals:
    void statUpdated(const Stat& stat);
  protected:
    virtual void run();
  private:
    void processIncomingStatReports();
//    StatsPipe* getStatsPipeForThread();
//    void onStatsPipeDestroyed(StatsPipe* pPipe);
    void writeTimeline(const QString& filename);
    std::atomic<int> m_resetStats{0};
    std::atomic<int> m_emitAllStats{0};
    std::atomic<int> m_quit{-1};
    QMap<QString, Stat> m_stats;
    QMap<QString, Stat> m_baseStats;
    QMap<QString, Stat> m_experimentStats;
    QSet<QString> m_tags;
    QList<Event> m_events;
//    QList<Event> m_events;

    intrusive_fifo<StatReport>  m_statsPipe{};
    std::atomic<size_t> m_pendingStats{0};
    mixxx::MSemaphore m_statsSema;
//    QMutex m_statsPipeLock;
//    QList<StatsPipe*> m_statsPipes;
//    QThreadStorage<StatsPipe*> m_threadStatsPipes;

    friend class StatsPipe;
};


#endif /* STATSMANAGER_H */
