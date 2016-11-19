#ifndef MIXXX_BPM_H
#define MIXXX_BPM_H

#include <QtDebug>

#include "util/math.h"

namespace mixxx {

// DTO for storing BPM information.
class Bpm final {
public:
    static constexpr double kValueUndefined = 0.0;
    static constexpr double kValueMin = 0.0; // lower bound (exclusive)

    constexpr Bpm() = default;
    constexpr Bpm(const Bpm &) noexcept = default;
    constexpr Bpm&operator=(const Bpm &) noexcept = default;
    constexpr Bpm(Bpm &&) noexcept = default;
    constexpr Bpm&operator=(Bpm &&) noexcept = default;

    explicit constexpr Bpm(double value) : m_value(value) { }
    explicit Bpm(const QString &str);

    static constexpr bool isValidValue(double value) { return kValueMin < value; }

    constexpr bool hasValue() const {return isValidValue(m_value);}
    constexpr double getValue() const { return m_value; }
    void setValue(double value) { m_value = value; }
    void resetValue() { m_value = kValueUndefined; }
    void normalizeValue();

    static double valueFromString(const QString& str, bool* pValid = nullptr);
    static QString valueToString(double value);
    static constexpr int valueToInteger(double value) { return std::round(value); }

    friend constexpr bool operator==(const Bpm&lhs, const Bpm&rhs)
    {
        return lhs.getValue() == rhs.getValue();
    }
    friend constexpr bool operator!=(const Bpm&lhs, const Bpm&rhs)
    {
        return !(lhs==rhs);
    }
    friend constexpr bool operator <(const Bpm&lhs, const Bpm&rhs)
    {
        return lhs.getValue() < rhs.getValue();
    }
    friend constexpr bool operator >(const Bpm&lhs, const Bpm&rhs)
    {
        return lhs.getValue() > rhs.getValue();
    }
    friend constexpr bool operator<=(const Bpm&lhs, const Bpm&rhs)
    {
        return lhs.getValue() <= rhs.getValue();
    }
    friend constexpr bool operator>=(const Bpm&lhs, const Bpm&rhs)
    {
        return lhs.getValue() >= rhs.getValue();
    }
private:
    double m_value{};
};
}
Q_DECLARE_METATYPE(mixxx::Bpm)
Q_DECLARE_TYPEINFO(mixxx::Bpm, Q_MOVABLE_TYPE);

#endif // MIXXX_BPM_H
