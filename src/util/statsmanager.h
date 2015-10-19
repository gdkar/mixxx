_Pragma("once")
#include <QMap>
#include <QObject>
#include <QString>
#include <QThread>
#include <QtDebug>
#include <QMutex>
#include <QWaitCondition>
#include <QThreadStorage>
#include <QList>

#include "util/fifo.h"
#include "util/singleton.h"
#include "util/stat.h"
#include "util/event.h"
#include <atomic>
class StatsManager;
class StatsPipe : public FIFO<StatReport> {
  public:
    StatsPipe(StatsManager* pManager);
    virtual ~StatsPipe();
  private:
    StatsManager* m_pManager = nullptr;
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
    void emitAllStats();
    void updateStats();
  signals:
    void statUpdated(const Stat& stat);
  protected:
    virtual void run();
  private:
    void processIncomingStatReports();
    StatsPipe* getStatsPipeForThread();
    void onStatsPipeDestroyed(StatsPipe* pPipe);
    void writeTimeline(QString filename);
    std::atomic<bool> m_emitAllStats{false};
    std::atomic<bool> m_quit{false};
    QMap<QString, Stat> m_stats;
    QMap<QString, Stat> m_baseStats;
    QMap<QString, Stat> m_experimentStats;
    QList<Event> m_events;
    QWaitCondition m_statsPipeCondition;
    QMutex m_statsPipeLock;
    QList<StatsPipe*> m_statsPipes;
    QThreadStorage<StatsPipe*> m_threadStatsPipes;
    friend class StatsPipe;
};
