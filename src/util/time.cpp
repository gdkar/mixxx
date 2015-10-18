#include "util/time.h"

// static
LLTIMER Time::s_timer;
void Time::start()
{
    s_timer.start();
}
qint64 Time::elapsed()
{
    return s_timer.elapsed();
}
uint Time::elapsedMsecs()
{
    return (uint)(s_timer.elapsed() / 1000);
}
QString Time::formatSeconds(double dSeconds, bool showCentis)
{
    if (dSeconds < 0) return "?";
    auto days = static_cast<int>(dSeconds / kSecondsPerDay);
    dSeconds -= days * kSecondsPerDay;
    auto t = QTime().addMSecs(dSeconds * kMillisPerSecond);
    auto formatString = QString(
            (days > 0 ? (QString::number(days) + QString("'d', ")) : QString()) +
            QString(days > 0 || t.hour() > 0 ? "hh:mm:ss" : "mm:ss") +
            QString(showCentis ? ".zzz" : ""));
    auto timeString = t.toString(formatString);
    // The format string gives us milliseconds but we want
    // centiseconds. Slice one character off.
    if (showCentis) timeString = timeString.left(timeString.length() - 1);
    return timeString;
}

