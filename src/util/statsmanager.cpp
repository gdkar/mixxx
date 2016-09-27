#include <QtDebug>
#include <QMutexLocker>
#include <QTextStream>
#include <QFile>
#include <QMetaType>

#include "util/types.h"
#include "util/statsmanager.h"
#include "util/compatibility.h"
#include "util/cmdlineargs.h"

// In practice we process stats pipes about once a minute @1ms latency.
const int kStatsPipeSize = 1 << 14;
//const int kProcessLength = kStatsPipeSize * 4 / 5;

// static
bool StatsManager::s_bStatsManagerEnabled = false;

/*StatsPipe::StatsPipe(StatsManager* pManager)
        : FIFO<StatReport>(kStatsPipeSize),
          m_pManager(pManager) {
    qRegisterMetaType<Stat>("Stat");
}

StatsPipe::~StatsPipe() {
    if (m_pManager) {
        m_pManager->onStatsPipeDestroyed(this);
    }
}*/

StatsManager::StatsManager()
        : QThread(),
          m_quit(0)
{
    qRegisterMetaType<Stat>("Stat");
    qRegisterMetaType<StatReport*>("StatReport*");
    s_bStatsManagerEnabled = true;
    setObjectName("StatsManager");
//    moveToThread(this);
    start(QThread::LowPriority);
}

StatsManager::~StatsManager()
{
    s_bStatsManagerEnabled = false;
    m_quit = 1;
    m_statsSema.release();
    wait();
    qDebug() << "StatsManager shutdown report:";
    qDebug() << "=====================================";
    qDebug() << "ALL STATS";
    qDebug() << "=====================================";
    for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
        qDebug() << it.value();
    }

    if (!m_baseStats.isEmpty()) {
        qDebug() << "=====================================";
        qDebug() << "BASE STATS";
        qDebug() << "=====================================";
        for (auto it = m_baseStats.begin(); it != m_baseStats.end(); ++it)
        {
            qDebug() << it.value();
        }
    }

    if (!m_experimentStats.isEmpty()) {
        qDebug() << "=====================================";
        qDebug() << "EXPERIMENT STATS";
        qDebug() << "=====================================";
        for (auto it = m_experimentStats.begin();
             it != m_experimentStats.end(); ++it)
        {
            qDebug() << it.value();
        }
    }
    qDebug() << "=====================================";

    if (CmdlineArgs::Instance().getTimelineEnabled()) {
        writeTimeline(CmdlineArgs::Instance().getTimelinePath());
    }
}

class OrderByTime {
  public:
    bool operator()(const Event& e1, const Event& e2) {
        return e1.m_time < e2.m_time;
    }
};

QString humanizeNanos(qint64 nanos) {
    auto seconds = static_cast<double>(nanos) / 1e9;
    if (seconds > 1) {
        return QString("%1s").arg(QString::number(seconds));
    }

    auto millis = static_cast<double>(nanos) / 1e6;
    if (millis > 1) {
        return QString("%1ms").arg(QString::number(millis));
    }

    auto micros = static_cast<double>(nanos) / 1e3;
    if (micros > 1) {
        return QString("%1us").arg(QString::number(micros));
    }

    return QString("%1ns").arg(QString::number(nanos));
}

void StatsManager::writeTimeline(const QString& filename)
{
    QFile timeline(filename);
    if (!timeline.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Could not open timeline file for writing:"
                 << timeline.fileName();
        return;
    }

    if (m_events.isEmpty()) {
        qDebug() << "No events recorded.";
        return;
    }

    // Sort by time.
    qSort(m_events.begin(), m_events.end(), OrderByTime());
    auto last_time = m_events[0].m_time;
    QMap<QString, qint64> startTimes;
    QMap<QString, qint64> endTimes;
    QMap<QString, Stat> tagStats;
    QTextStream out(&timeline);
    for(auto && event: as_const(m_events)) {
        auto last_start = startTimes.value(event.m_tag, -1);
        auto last_end = endTimes.value(event.m_tag, -1);

        auto duration_since_last_start = last_start == -1 ? 0 :
                event.m_time.toIntegerNanos() - last_start;
        auto duration_since_last_end = last_end == -1 ? 0 :
                event.m_time.toIntegerNanos() - last_end;

        if (event.m_type == Stat::EVENT_START) {
            // We last saw a start and we just saw another start.
            if (last_start > last_end) {
                qDebug() << "Mismatched start/end pair" << event.m_tag;
            }
            startTimes[event.m_tag] = event.m_time.toIntegerNanos();
        } else if (event.m_type == Stat::EVENT_END) {
            // We last saw an end and we just saw another end.
            if (last_end > last_start) {
                qDebug() << "Mismatched start/end pair" << event.m_tag;
            }
            endTimes[event.m_tag] = event.m_time.toIntegerNanos();
        }
        // TODO(rryan): CSV escaping
        auto elapsed = (event.m_time - last_time).toIntegerNanos();
        out << event.m_time.toIntegerNanos() << ","
            << "+" << humanizeNanos(elapsed) << ","
            << "+" << humanizeNanos(duration_since_last_start) << ","
            << "+" << humanizeNanos(duration_since_last_end) << ","
            << Stat::statTypeToString(event.m_type) << ","
            << event.m_tag << "\n";
        last_time = event.m_time;
    }

    timeline.close();
}

/*void StatsManager::onStatsPipeDestroyed(StatsPipe* pPipe)
{
    QMutexLocker locker(&m_statsPipeLock);
    processIncomingStatReports();
    m_statsPipes.removeAll(pPipe);
}*/

/*StatsPipe* StatsManager::getStatsPipeForThread() {
    if (m_threadStatsPipes.hasLocalData()) {
        return m_threadStatsPipes.localData();
    }
    auto pResult = new StatsPipe(this);
    m_threadStatsPipes.setLocalData(pResult);
    QMutexLocker locker(&m_statsPipeLock);
    m_statsPipes.push_back(pResult);
    return pResult;
}*/

bool StatsManager::maybeWriteReport(std::unique_ptr<StatReport>& report)
{
    if(report) {
        m_statsPipe.push(report.release());
        auto pending = m_pendingStats.fetch_add(1);
        if(pending > kStatsPipeSize) {
            auto swapped = m_pendingStats.exchange(0);
            if(swapped > kStatsPipeSize) {
                m_statsSema.release();
            }else{
                m_pendingStats.fetch_add(swapped);
            }
        }
        return true;
    }else{
        return false;
    }
}
bool StatsManager::maybeWriteReport(StatReport* report)
{
    if(report) {
        m_statsPipe.push(report);
        auto pending = m_pendingStats.fetch_add(1);
        if(pending > kStatsPipeSize) {
            auto swapped = m_pendingStats.exchange(0);
            if(swapped > kStatsPipeSize) {
                m_statsSema.release();
            }else{
                m_pendingStats.fetch_add(swapped);
            }
        }
        return true;
    }else{
        return false;
    }
}

void StatsManager::processIncomingStatReports()
{
    while(!m_statsPipe.empty()) {
        auto *stat = m_statsPipe.take();
        if(!stat) {
            break;
        }
        auto &&tag  = stat->tag;
        auto & info = m_stats[tag];
        info.m_tag  = stat->tag;
        info.m_type = stat->type;
        info.m_compute = stat->compute;
        info.processReport(*stat);
        emit(statUpdated(info));

        if (stat->compute & Stat::STATS_EXPERIMENT) {
            auto& experiment = m_experimentStats[tag];
            experiment.m_tag = tag;
            experiment.m_type = stat->type;
            experiment.m_compute = stat->compute;
            experiment.processReport(*stat);
        } else if (stat->compute & Stat::STATS_BASE) {
            auto & base = m_baseStats[tag];
            base.m_tag = tag;
            base.m_type = stat->type;
            base.m_compute = stat->compute;
            base.processReport(*stat);
        }
        if (CmdlineArgs::Instance().getTimelineEnabled() &&
                (stat->type == Stat::EVENT ||
                    stat->type == Stat::EVENT_START ||
                    stat->type == Stat::EVENT_END)) {
            Event event;
            event.m_tag = tag;
            event.m_type = stat->type;
            event.m_time = mixxx::Duration::fromNanos(stat->time);
            m_events.append(event);
        }
        delete stat;
    }
}

void StatsManager::run()
{
    qDebug() << "StatsManager thread starting up.";
    while (true) {
        m_statsSema.acquire();
        processIncomingStatReports();
        if (m_resetStats.exchange(0) == 1) {
            for(auto it = m_stats.begin(); it != m_stats.end(); ++it)
                it.value().clear();
        }
        if (m_emitAllStats.exchange(0) == 1) {
            for (auto it = m_stats.cbegin(); it != m_stats.cend(); ++it)
                emit(statUpdated(it.value()));
        }
        if (m_quit.load() == 1) {
            qDebug() << "StatsManager thread shutting down.";
            break;
        }
    }
}
