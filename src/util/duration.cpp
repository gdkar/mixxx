#include "util/duration.h"

#include <QtGlobal>
#include <QStringBuilder>
#include <QTime>

#include "util/assert.h"
#include <util/math.h>

namespace mixxx {

namespace {

static constexpr const qint64 kSecondsPerMinute = 60;
static constexpr const qint64 kSecondsPerHour = 60 * kSecondsPerMinute;
static constexpr const qint64 kSecondsPerDay = 24 * kSecondsPerHour;

} // namespace

// static
QString DurationBase::formatSeconds(double dSeconds, Precision precision)
{
    if (dSeconds < 0.0) {
        // negative durations are not supported
        return QString{"?"};
    }
    auto days = static_cast<qint64>(std::floor(dSeconds)) / kSecondsPerDay;
    dSeconds -= days * kSecondsPerDay;

    // NOTE(uklotzde): QTime() constructs a 'null' object, but
    // we need 'zero' here.
    QTime t = QTime(0, 0).addMSecs(dSeconds * kMillisPerSecond);

    auto formatString = QString(
            (days > 0 ? (QString::number(days) +
                         QString("'d', ")) : QString("")) +
            QString(days > 0 || t.hour() > 0 ? "hh:mm:ss" : "mm:ss") +
            QString(Precision::SECONDS == precision ? "" : ".zzz"));

    auto durationString = t.toString(formatString);

    // The format string gives us milliseconds but we want
    // centiseconds. Slice one character off.
    if (Precision::CENTISECONDS == precision) {
        DEBUG_ASSERT(1 <= durationString.length());
        durationString = durationString.left(durationString.length() - 1);
    }
    return durationString;
}
    // Formats the duration as a two's-complement hexadecimal string.
QString Duration::formatHex() const {
    // Format as fixed-width (8 digits).
    return QString("0x%1").arg(m_durationNanos, 16, 16, QLatin1Char('0'));
}
QString Duration::formatNanosWithUnit() const {
    return QString("%1 ns").arg(toIntegerNanos());
}
QString Duration::formatMicrosWithUnit() const {
    return QString("%1 us").arg(toIntegerMicros());
}
QString Duration::formatMillisWithUnit() const {
    return QString("%1 ms").arg(toIntegerMillis());
}
QString Duration::formatSecondsWithUnit() const {
    return QString("%1 s").arg(toIntegerSeconds());
}

} // namespace mixxx
