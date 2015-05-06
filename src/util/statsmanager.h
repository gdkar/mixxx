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
#include "util/ff_ringbuffer.h"
#include "util/reference.h"
#include "util/singleton.h"
#include "util/stat.h"
#include "util/event.h"

class StatsManager;
const int kStatsPipeSize = 1 << 20;
const int kProcessLength = kStatsPipeSize / 2;

class StatsPipe : public FFItemBuffer<StatReport,kStatsPipeSize> {
  public:
    StatsPipe();
    virtual ~StatsPipe();
  private:
    StatsManager* m_pManager;
};

class StatsManager : public QThread, public Singleton<StatsManager> {
    Q_OBJECT
    typedef FreeList<StatsPipe>   fl_type;
    typedef FreeList<StatsPipe>::Link fl_link_type;
    typedef FreeListIterator<StatsPipe> fl_iter_type;
    typedef FreeListHolder<StatsPipe> fl_holder_type;
  public:
    explicit StatsManager();
    virtual ~StatsManager();

    // Returns true if write succeeds.
    bool maybeWriteReport(const StatReport& report);

    static bool s_bStatsManagerEnabled;

    // Tell the StatsManager to emit statUpdated for every stat that exists.
    void emitAllStats() {
        m_emitAllStats = 1;
    }

    void updateStats() {
        m_statsPipeCondition.wakeAll();
    }

  signals:
    void statUpdated(const Stat& stat);

  protected:
    virtual void run();

  private:
    fl_type       m_freeList;
    void processIncomingStatReports();
    StatsPipe* getStatsPipeForThread();
    void onStatsPipeDestroyed(StatsPipe* pPipe);
    void writeTimeline(const QString& filename);

    QAtomicInt m_emitAllStats;
    QAtomicInt m_quit;
    QMap<QString, Stat> m_stats;
    QMap<QString, Stat> m_baseStats;
    QMap<QString, Stat> m_experimentStats;
    QList<Event> m_events;

    QWaitCondition m_statsPipeCondition;
    QMutex m_statsPipeLock;
    QList<StatsPipe*> m_statsPipes;
    QThreadStorage<fl_holder_type*> m_threadStatsPipes;

    friend class StatsPipe;
};


#endif /* STATSMANAGER_H */
