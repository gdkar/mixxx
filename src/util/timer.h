_Pragma("once")
#include "util/stat.h"
#include "util/performancetimer.h"
#include "util/cmdlineargs.h"

const Stat::ComputeFlags kDefaultComputeFlags = Stat::COUNT | Stat::SUM | Stat::AVERAGE | Stat::MAX | Stat::MIN | Stat::SAMPLE_VARIANCE;

// A Timer that is instrumented for reporting elapsed times to StatsManager
// under a certain key. Construct with custom compute flags to get custom values
// computed for the times.
class Timer {
  public:
    Timer(QString key, Stat::ComputeFlags compute = kDefaultComputeFlags);
    virtual ~Timer();
    virtual void start();
    // Restart the timer returning the nanoseconds since it was last
    // started/restarted. If report is true, reports the elapsed time to the
    // associated Stat key.
    virtual qint64 restart(bool report);
    // Returns nanoseconds since start/restart was called. If report is true,
    // reports the elapsed time to the associated Stat key.
    virtual qint64 elapsed(bool report);
  protected:
    QString m_key;
    Stat::ComputeFlags m_compute;
    bool m_running = false;
    PerformanceTimer m_time;
};
class SuspendableTimer : public Timer
{
  public:
    SuspendableTimer(QString key,Stat::ComputeFlags compute = kDefaultComputeFlags);
    virtual ~SuspendableTimer();
    virtual void start();
    virtual qint64 suspend();
    virtual void go();
    virtual qint64 elapsed(bool report);
  private:
    qint64 m_leapTime = 0;
};
class ScopedTimer
{
  public:
    ScopedTimer(const char* key, int i,Stat::ComputeFlags compute = kDefaultComputeFlags);
    ScopedTimer(const char* key, const char *arg = nullptr,Stat::ComputeFlags compute = kDefaultComputeFlags);
    ScopedTimer(const char* key, QString arg,Stat::ComputeFlags compute = kDefaultComputeFlags);
    virtual ~ScopedTimer();
    virtual void initialize(QString key, QString arg,Stat::ComputeFlags compute = kDefaultComputeFlags);
    virtual void cancel();
  private:
    Timer* m_pTimer = nullptr;
    char m_timerMem[sizeof(Timer)];
    bool m_cancel = false;
};
