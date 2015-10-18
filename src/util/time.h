_Pragma("once")
#include <QtGlobal>
#include <QString>
#include <QDateTime>
#include <QTime>
#include <QStringBuilder>

#include "util/performancetimer.h"
#include "util/timer.h"

#define LLTIMER PerformanceTimer
//#define LLTIMER ThreadCpuTimer

class Time {
  public:
    static const int kMillisPerSecond = 1000;
    static const int kSecondsPerMinute = 60;
    static const int kSecondsPerHour = 60 * kSecondsPerMinute;
    static const int kSecondsPerDay = 24 * kSecondsPerHour;

    static void start();
    // Returns the time elapsed since Mixxx started up in nanoseconds.
    static qint64 elapsed();
    static uint elapsedMsecs();
    // The standard way of formatting a time in seconds. Used for display of
    // track duration, etc. showCentis indicates whether to include
    // centisecond-precision or to round to the nearest second.
    static QString formatSeconds(double dSeconds, bool showCentis);
  private:
    static LLTIMER s_timer;
};
