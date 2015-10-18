#include "util/timer.h"
#include "util/experiment.h"

Timer::Timer(const QString& key, Stat::ComputeFlags compute)
        : m_key(key),
          m_compute(Stat::experimentFlags(compute)),
          m_running(false)
{
}
void Timer::start()
{
    m_running = true;
    m_time.start();
}
Timer::~Timer() = default;
qint64 Timer::restart(bool report)
{
    if (m_running)
    {
        auto nsec = m_time.restart();
        if (report)
        {
            // Ignore the report if it crosses the experiment boundary.
            auto oldMode = Stat::modeFromFlags(m_compute);
            if (oldMode == Experiment::mode()) Stat::track(m_key, Stat::DURATION_NANOSEC, m_compute, nsec);
        }
        return nsec;
    }
    else
    {
        start();
        return 0;
    }
}
qint64 Timer::elapsed(bool report)
{
    auto nsec = m_time.elapsed();
    if (report)
    {
        // Ignore the report if it crosses the experiment boundary.
        auto oldMode = Stat::modeFromFlags(m_compute);
        if (oldMode == Experiment::mode()) Stat::track(m_key, Stat::DURATION_NANOSEC, m_compute, nsec);
    }
    return nsec;
}
SuspendableTimer::SuspendableTimer(const QString& key,Stat::ComputeFlags compute)
        : Timer(key, compute),
          m_leapTime(0)
{
}
SuspendableTimer::~SuspendableTimer() = default;
void SuspendableTimer::start()
{
    m_leapTime = 0;
    Timer::start();
}
qint64 SuspendableTimer::suspend()
{
    m_leapTime += m_time.elapsed();
    m_running = false;
    return m_leapTime;
}
void SuspendableTimer::go()
{
    Timer::start();
}
qint64 SuspendableTimer::elapsed(bool report)
{
    m_leapTime += m_time.elapsed();
    if (report) {
        // Ignore the report if it crosses the experiment boundary.
        auto oldMode = Stat::modeFromFlags(m_compute);
        if (oldMode == Experiment::mode()) Stat::track(m_key, Stat::DURATION_NANOSEC, m_compute, m_leapTime);
    }
    return m_leapTime;
}
ScopedTimer::ScopedTimer(const char* key, int i,Stat::ComputeFlags compute )
        : m_pTimer(nullptr),
          m_cancel(false)
{
    if (CmdlineArgs::Instance().getDeveloper()) initialize(QString(key), QString::number(i), compute);
}
ScopedTimer::ScopedTimer(const char* key, const char *arg,Stat::ComputeFlags compute )
        : m_pTimer(nullptr),
          m_cancel(false)
{
    if (CmdlineArgs::Instance().getDeveloper()) initialize(QString(key), arg ? QString(arg) : QString(), compute);
}
ScopedTimer::ScopedTimer(const char* key, const QString& arg,Stat::ComputeFlags compute)
        : m_pTimer(nullptr),
          m_cancel(false)
{
    if (CmdlineArgs::Instance().getDeveloper()) initialize(QString(key), arg, compute);
}
ScopedTimer::~ScopedTimer()
{
    if (m_pTimer)
    {
        if (!m_cancel) m_pTimer->elapsed(true);
        m_pTimer->~Timer();
    }
}
void ScopedTimer::initialize(const QString& key, const QString& arg,Stat::ComputeFlags compute )
{
    QString strKey;
    if (arg.isEmpty()) strKey = key;
    else               strKey = key.arg(arg);
    m_pTimer = new(m_timerMem) Timer(strKey, compute);
    m_pTimer->start();
}
void ScopedTimer::cancel()
{
    m_cancel = true;
}

