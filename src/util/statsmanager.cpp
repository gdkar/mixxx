#include <QtDebug>
#include <QTextStream>
#include <QFile>
#include <QMetaType>

#include "util/statsmanager.h"
#include "util/compatibility.h"
#include "util/cmdlineargs.h"

// In practice we process stats pipes about once a minute @1ms latency.

// static
bool StatsManager::s_bStatsManagerEnabled = false;

StatsPipe::StatsPipe()
        : FFItemBuffer<StatReport,kStatsPipeSize>(),
          m_pManager(StatsManager::instance()) {
    qRegisterMetaType<Stat>("Stat");
}

StatsPipe::~StatsPipe() {}

StatsManager::StatsManager()
        : QThread(),
          m_quit(0) {
    s_bStatsManagerEnabled = true;
    setObjectName("StatsManager");
    moveToThread(this);
    start(QThread::LowPriority);
}

StatsManager::~StatsManager() {
    s_bStatsManagerEnabled = false;
    m_quit = 1;
    m_statsPipeSem.release();
    wait();
    qDebug() << "StatsManager shutdown report:";
    qDebug() << "=====================================";
    qDebug() << "ALL STATS";
    qDebug() << "=====================================";
    for (QMap<QString, Stat>::const_iterator it = m_stats.begin();
         it != m_stats.end(); ++it) {
        qDebug() << it.value();
    }

    if (!m_baseStats.isEmpty()) {
        qDebug() << "=====================================";
        qDebug() << "BASE STATS";
        qDebug() << "=====================================";
        for (QMap<QString, Stat>::const_iterator it = m_baseStats.begin();
             it != m_baseStats.end(); ++it) {
            qDebug() << it.value();
        }
    }

    if (!m_experimentStats.isEmpty()) {
        qDebug() << "=====================================";
        qDebug() << "EXPERIMENT STATS";
        qDebug() << "=====================================";
        for (QMap<QString, Stat>::const_iterator it = m_experimentStats.begin();
             it != m_experimentStats.end(); ++it) {
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
    inline bool operator()(const Event& e1, const Event& e2) {
        return e1.m_time < e2.m_time;
    }
};

QString humanizeNanos(qint64 nanos) {
    double seconds = static_cast<double>(nanos) / 1e9;
    if (seconds > 1) {
        return QString("%1s").arg(QString::number(seconds));
    }

    double millis = static_cast<double>(nanos) / 1e6;
    if (millis > 1) {
        return QString("%1ms").arg(QString::number(millis));
    }

    double micros = static_cast<double>(nanos) / 1e3;
    if (micros > 1) {
        return QString("%1us").arg(QString::number(micros));
    }

    return QString("%1ns").arg(QString::number(nanos));
}

void StatsManager::writeTimeline(const QString& filename) {
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

    qint64 last_time = m_events[0].m_time;

    QMap<QString, qint64> startTimes;
    QMap<QString, qint64> endTimes;
    QMap<QString, Stat> tagStats;

    QTextStream out(&timeline);
    foreach (const Event& event, m_events) {
        qint64 last_start = startTimes.value(event.m_tag, -1);
        qint64 last_end = endTimes.value(event.m_tag, -1);

        qint64 duration_since_last_start = last_start == -1 ? 0 : event.m_time - last_start;
        qint64 duration_since_last_end = last_end == -1 ? 0 : event.m_time - last_end;

        if (event.m_type == Stat::EVENT_START) {
            // We last saw a start and we just saw another start.
            if (last_start > last_end) {qDebug() << "Mismatched start/end pair" << event.m_tag;}
            startTimes[event.m_tag] = event.m_time;
        } else if (event.m_type == Stat::EVENT_END) {
            // We last saw an end and we just saw another end.
            if (last_end > last_start) {qDebug() << "Mismatched start/end pair" << event.m_tag;}
            endTimes[event.m_tag] = event.m_time;
        }
        qint64 elapsed = event.m_time - last_time;
        QString mtag(event.m_tag);
        mtag.replace("\"","\"\"");
        out << event.m_time << ","
            << "+" << humanizeNanos(elapsed) << ","
            << "+" << humanizeNanos(duration_since_last_start) << ","
            << "+" << humanizeNanos(duration_since_last_end) << ","
            << Stat::statTypeToString(event.m_type) << ","
            <<"\"" <<mtag << "\"\n";
        last_time = event.m_time;
    }
    timeline.close();
}


StatsPipe* StatsManager::getStatsPipeForThread() {
    if (Q_UNLIKELY(!m_threadStatsPipes.hasLocalData())) {
      fl_holder_type * pResult = new fl_holder_type(&m_freeList);
      m_threadStatsPipes.setLocalData(pResult);
    }
    return m_threadStatsPipes.localData()->item();
}

bool StatsManager::maybeWriteReport(const StatReport& report) {
    StatsPipe* pStatsPipe = getStatsPipeForThread();
    if (pStatsPipe == NULL) {
        return false;
    }
    bool success = pStatsPipe->write(report) ;
    qint64 count = pStatsPipe->count();
    if (((count % kProcessLength)==0)) {m_statsPipeSem.release();}
    return success;
}

void StatsManager::processIncomingStatReports() {
    StatReport report;
    fl_iter_type pipes(m_freeList);
    while(pipes.hasNext()){
        StatsPipe &rStatsPipe = pipes.next();
        while (rStatsPipe.read(report)) {
            QString tag = QString::fromUtf8(report.tag);
            Stat& info = m_stats[tag];
            info.m_tag = tag;
            info.m_type = report.type;
            info.m_compute = report.compute;
            info.processReport(report);
            emit(statUpdated(info));

            if (report.compute & Stat::STATS_EXPERIMENT) {
                Stat& experiment = m_experimentStats[tag];
                experiment.m_tag = tag;
                experiment.m_type = report.type;
                experiment.m_compute = report.compute;
                experiment.processReport(report);
            } else if (report.compute & Stat::STATS_BASE) {
                Stat& base = m_baseStats[tag];
                base.m_tag = tag;
                base.m_type = report.type;
                base.m_compute = report.compute;
                base.processReport(report);
            }

            if (CmdlineArgs::Instance().getTimelineEnabled() &&
                    (report.type == Stat::EVENT ||
                     report.type == Stat::EVENT_START ||
                     report.type == Stat::EVENT_END)) {
                Event event;
                event.m_tag = tag;
                event.m_type = report.type;
                event.m_time = report.time;
                m_events.append(event);
            }
            free(report.tag);
        }
    }
}

void StatsManager::run() {
    qDebug() << "StatsManager thread starting up.";
    while (true) {
        m_statsPipeSem.acquire();
        // We want to process reports even when we are about to quit since we
        // want to print the most accurate stat report on shutdown.
        processIncomingStatReports();
        if (load_atomic(m_emitAllStats) == 1) {
            for (QMap<QString, Stat>::const_iterator it = m_stats.begin();
                 it != m_stats.end(); ++it) {
                emit(statUpdated(it.value()));
            }
            m_emitAllStats = 0;
        }

        if (load_atomic(m_quit) == 1) {
            qDebug() << "StatsManager thread shutting down.";
            break;
        }
    }
}
