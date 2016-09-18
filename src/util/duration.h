#ifndef MIXXX_UTIL_DURATION_H
#define MIXXX_UTIL_DURATION_H

#include <QMetaType>
#include <QString>
#include <QObject>
#include <QMetaObject>
#include <QMetaType>
#include <QtDebug>
#include <QtGlobal>
#include <QTextStreamFunction>

#include "util/assert.h"

namespace mixxx {

class DurationBase {

  public:
    enum Units {
        HEX,
        SECONDS,
        MILLIS,
        MICROS,
        NANOS
    };

    // Returns the duration as an integer number of seconds (rounded-down).
    constexpr qint64 toIntegerSeconds() const {
        return m_durationNanos / kNanosPerSecond;
    }

    // Returns the duration as a floating point number of seconds.
    constexpr double toDoubleSeconds() const {
        return static_cast<double>(m_durationNanos) / kNanosPerSecond;
    }

    // Returns the duration as an integer number of milliseconds (rounded-down).
    constexpr qint64 toIntegerMillis() const {
        return m_durationNanos / kNanosPerMilli;
    }

    // Returns the duration as a floating point number of milliseconds.
    constexpr qint64 toDoubleMillis() const {
        return static_cast<double>(m_durationNanos) / kNanosPerMilli;
    }

    // Returns the duration as an integer number of microseconds (rounded-down).
    constexpr qint64 toIntegerMicros() const {
        return m_durationNanos / kNanosPerMicro;
    }

    // Returns the duration as a floating point number of microseconds.
    constexpr qint64 toDoubleMicros() const {
        return static_cast<double>(m_durationNanos) / kNanosPerMicro;
    }

    // Returns the duration as an integer number of nanoseconds. The duration is
    // represented internally as nanoseconds so no rounding occurs.
    constexpr qint64 toIntegerNanos() const {
        return m_durationNanos;
    }

    // Returns the duration as an integer number of nanoseconds.
    constexpr qint64 toDoubleNanos() const {
        return static_cast<double>(m_durationNanos);
    }
    enum class Precision {
        SECONDS,
        CENTISECONDS,
        MILLISECONDS
    };
    // The standard way of formatting a floating-point duration in seconds.
    // Used for display of track duration, etc.
    static QString formatSeconds(
            double dSeconds,
            Precision precision = Precision::SECONDS);

    static constexpr qint64 kMillisPerSecond = 1000;
    static constexpr qint64 kMicrosPerSecond = kMillisPerSecond * 1000;
    static constexpr qint64 kNanosPerSecond  = kMicrosPerSecond * 1000;
    static constexpr qint64 kNanosPerMilli   = kNanosPerSecond / 1000;
    static constexpr qint64 kNanosPerMicro   = kNanosPerMilli / 1000;

  protected:
    constexpr DurationBase(qint64 durationNanos)
        : m_durationNanos(durationNanos){ }
    qint64 m_durationNanos{};
};

class DurationDebug : public DurationBase {
  public:
    constexpr DurationDebug(const DurationBase& duration, Units unit)
        : DurationBase(duration),
          m_unit(unit) {
    }
    friend QDebug operator<<(QDebug debug, const DurationDebug& dd)
    {
        switch (dd.m_unit) {
        case HEX:
        {
            auto ret = QByteArray("0x0000000000000000");
            auto hex = QByteArray::number(dd.m_durationNanos, 16);
            ret.replace(18 - hex.size(), hex.size(), hex);
            return debug << ret;
        }
        case SECONDS:   return debug << dd.toIntegerSeconds() << "s";
        case MILLIS:    return debug << dd.toIntegerMillis() << "ms";
        case MICROS:    return debug << dd.toIntegerMicros() << "us";
        case NANOS:     return debug << dd.m_durationNanos << "ns";
        default:
            DEBUG_ASSERT(!"Unit unknown");
                        return debug << dd.m_durationNanos << "ns";
        }
    }
  private:
    Units m_unit;
};
// Represents a duration in a type-safe manner. Provides conversion methods to
// convert between physical units. Durations can be negative.
class Duration : public DurationBase {
  public:
    // Returns a Duration object representing a duration of 'seconds'.
    static constexpr  Duration fromSeconds(qint64 seconds) {
        return Duration(seconds * kNanosPerSecond);
    }
    // Returns a Duration object representing a duration of 'millis'.
    static constexpr Duration fromMillis(qint64 millis) {
        return Duration(millis * kNanosPerMilli);
    }
    // Returns a Duration object representing a duration of 'micros'.
    static constexpr Duration fromMicros(qint64 micros) {
        return Duration(micros * kNanosPerMicro);
    }
    // Returns a Duration object representing a duration of 'nanos'.
    static constexpr Duration fromNanos(qint64 nanos) {
        return Duration(nanos);
    }
    constexpr Duration()
        : DurationBase(0) {
    }
    const constexpr Duration operator+(const Duration& other) const
    {
        auto result = *this;
        result += other;
        return result;
    }
    constexpr Duration& operator+=(const Duration& other) {
        m_durationNanos += other.m_durationNanos;
        return *this;
    }
    constexpr Duration operator-(const Duration& other) const {
        auto result = *this;
        result -= other;
        return result;
    }
    constexpr Duration& operator-=(const Duration& other) {
        m_durationNanos -= other.m_durationNanos;
        return *this;
    }
    friend constexpr Duration operator*(const Duration& duration, int scalar) {
        auto result = duration;
        result.m_durationNanos *= scalar;
        return result;
    }
    friend constexpr Duration operator*(int scalar, const Duration& duration) {
        return duration * scalar;
    }
    constexpr Duration& operator*=(int scalar) {
        m_durationNanos *= scalar;
        return *this;
    }
    friend constexpr bool operator==(const Duration& lhs, const Duration& rhs) {
        return lhs.m_durationNanos == rhs.m_durationNanos;
    }
    friend constexpr bool operator!=(const Duration& lhs, const Duration& rhs) {
        return !(lhs == rhs);
    }
    friend constexpr bool operator<(const Duration& lhs, const Duration& rhs) {
        return lhs.m_durationNanos < rhs.m_durationNanos;
    }
    friend constexpr bool operator>(const Duration& lhs, const Duration& rhs) {
        return lhs.m_durationNanos > rhs.m_durationNanos;
    }
    friend constexpr bool operator<=(const Duration& lhs, const Duration& rhs) {
        return lhs.m_durationNanos <= rhs.m_durationNanos;
    }
    friend constexpr bool operator>=(const Duration& lhs, const Duration& rhs) {
        return lhs.m_durationNanos >= rhs.m_durationNanos;
    }
    friend QDebug operator<<(QDebug debug, const Duration& duration) {
        return debug << duration.m_durationNanos << "ns";
    }
    // Formats the duration as a two's-complement hexadecimal string.
    QString formatHex() const;
    // Formats the duration as a two's-complement hexadecimal string.
    constexpr DurationDebug debugHex() const {
        return debug(HEX);
    }
    QString formatNanosWithUnit() const;
    constexpr DurationDebug debugNanosWithUnit() const {
        return debug(NANOS);
    }
    QString formatMicrosWithUnit() const;
    constexpr DurationDebug debugMicrosWithUnit() const {
        return debug(MICROS);
    }
    QString formatMillisWithUnit() const ;
    constexpr DurationDebug debugMillisWithUnit() const {
        return debug(MILLIS);
    }
    QString formatSecondsWithUnit() const;
    constexpr DurationDebug debugSecondsWithUnit() const {
        return debug(SECONDS);
    }
    constexpr DurationDebug debug(Units unit) const {
        return DurationDebug(*this, unit);
    }
  private:
    constexpr Duration(qint64 durationNanos)
            : DurationBase(durationNanos) {
    }
};
}  // namespace mixxx
Q_DECLARE_METATYPE(mixxx::Duration)
#endif /* MIXXX_UTIL_DURATION_H */
