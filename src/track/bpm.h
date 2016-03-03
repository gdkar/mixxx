_Pragma("once")
#include <QtDebug>

#include "util/math.h"

namespace Mixxx {

// DTO for storing BPM information.
class Bpm final {
public:
    static const double kValueUndefined;
    static const double kValueMin; // lower bound (exclusive)
    Bpm();
    explicit Bpm(double value);
    static bool isValidValue(double value);
    bool hasValue() const;
    double getValue() const;
    void setValue(double value);
    void resetValue();
    void normalizeValue();
    static double valueFromString(const QString& str, bool* pValid = nullptr);
    static QString valueToString(double value);
    static int valueToInteger(double value);
private:
    double m_value{kValueUndefined};
};
bool operator==(const Bpm& lhs, const Bpm& rhs);
bool operator!=(const Bpm& lhs, const Bpm& rhs);
}
Q_DECLARE_METATYPE(Mixxx::Bpm)
