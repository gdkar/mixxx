_Pragma("once")
#include <ladspa.h>
#include <QHash>
#include <QString>
#include <QObject>
#include <QMetaType>
#include <QtQml>
#include <QtCore>
#include <QtGlobal>

class ControlObject;

struct ControlHint {
    enum struct Default : int {
        None    = 0,
        Minimum = 0x040,
        Low     = 0x080,
        Middle  = 0x0C0,
        High    = 0x100,
        Maximum = 0x140,
        Zero    = 0x200,
        One     = 0x240,
        Hundred = 0x280,
        FourForty = 0x2C0,
    };
    Q_ENUM(Default)

    Q_PROPERTY(double lowerBound READ lowerBound WRITE setLowerBound)
    Q_PROPERTY(double upperBound READ upperBound WRITE setUpperBound)
    Q_PROPERTY(double default READ defaultValue)
    Q_PROPERTY(bool boundedBelow READ boundedBelow )
    Q_PROPERTY(bool boundedAbove READ boundedAbove )
    Q_PROPERTY(bool toggled READ toggled WRITE setToggled)
    Q_PROPERTY(bool sampleRate READ sampleRate WRITE setSampleRate)
    Q_PROPERTY(bool logarithmic READ logarithmic WRITE setLogarithmic)
    Q_PROPERTY(bool integer READ integer WRITE setInteger)
    Q_PROPERTY(bool hasDefault READ hasDefault)
    Q_PROPERTY(Default defaultType READ defaultType WRITE setDefaultType)
    Q_GADGET
public:
    int m_desc{0};
    double m_lower = -std::numeric_limits<double>::infinity();
    double m_upper =  std::numeric_limits<double>::infinity();

    constexpr ControlHint() = default;
    constexpr ControlHint(const ControlHint&) = default;
    constexpr ControlHint(ControlHint&&) noexcept = default;
    constexpr ControlHint&operator=(const ControlHint&) = default;
    constexpr ControlHint&operator=(ControlHint&&) noexcept = default;

    double lowerBound() const;
    void   setLowerBound(double val);
    double upperBound() const;
    void   setUpperBound(double val);
    double defaultValue() const;
    bool   boundedBelow() const;
    bool   boundedAbove() const;
    bool   toggled() const;
    void   setToggled(bool val);
    bool   sampleRate() const;
    void   setSampleRate(bool val);
    bool   logarithmic() const;
    void   setLogarithmic(bool val);
    bool   integer() const;
    void   setInteger(bool val);
    bool   hasDefault() const;
    Default defaultType() const;
    void   setDefaultType(Default val);

    double proportionToValue(double val) const;
    double valueToProportion(double val) const;

    friend bool operator == ( const ControlHint&lhs, const ControlHint &rhs)
    {
        return lhs.m_desc == rhs.m_desc
            && lhs.upperBound() == rhs.upperBound()
            && lhs.lowerBound() == rhs.lowerBound();
    }
    friend bool operator != ( const ControlHint&lhs, const ControlHint &rhs)
    {
        return !(lhs == rhs);
    }
};

QML_DECLARE_TYPE(ControlHint);
