_Pragma("once")

#include <QLibrary>
#include <QObject>
#include <QtGlobal>
#include <QtDebug>
#include <QtQml>
#include <QtQuick>
#include <QMetaEnum>
#include <QMetaType>

extern "C" {
#include <ladspa.h>
};

namespace {
    template<class T>
    constexpr std::enable_if_t<std::is_enum<T>::value,T> bitmask_set(T src, T bit, bool val)
    {
        using U = std::underlying_type_t<T>;
        return T(((U(src) & ~U(bit))|U(bit)) | (val ? U(bit) : U()));
    }
    template<class T>
    constexpr std::enable_if_t<std::is_enum<T>::value,T> bitmask_set(std::underlying_type_t<T> src, T bit, bool val)
    {
        using U = std::underlying_type_t<T>;
        return T(((src & ~U(bit))|U(bit)) | (val ? U(bit) : U()));
    }
    template<class T>
    constexpr std::enable_if_t<!std::is_enum<T>::value,T> bitmask_set(T src, T bit, bool val)
    {
        return (src & ~bit) | (val ? bit : T{});
    }
};
struct RangeHint : LADSPA_PortRangeHint {
    Q_GADGET
    Q_PROPERTY(bool boundedBelow READ boundedBelow WRITE setBoundedBelow)
    Q_PROPERTY(bool boundedAbove READ boundedAbove WRITE setBoundedAbove)
    Q_PROPERTY(bool toggled READ toggled WRITE setToggled)
    Q_PROPERTY(bool sampleRate READ sampleRate WRITE setSampleRate)
    Q_PROPERTY(bool logarithmic READ logarithmic WRITE setLogarithmic)
    Q_PROPERTY(bool integer READ integer WRITE setInteger);
    Q_PROPERTY(DefaultType defaultType READ defaultType WRITE setDefaultType)
    Q_PROPERTY(float lowerBound READ lowerBound WRITE setLowerBound)
    Q_PROPERTY(float upperBound READ upperBound WRITE setUpperBound)
    Q_PROPERTY(float defaultValue READ defaultValue)
public:
    enum struct DefaultType : int {
        Mask = 0x3c0,
        None = 0x000,
        Minimum = 0x40,
        Low     = 0x80,
        Middle  = 0xC0,
        High    = 0x100,
        Maximum = 0x140,
        Zero    = 0x200,
        One     = 0x240,
        OneHundred = 0x280,
        FourForty  = 0x2c0,
    };
    Q_ENUM(DefaultType);
    constexpr RangeHint() : LADSPA_PortRangeHint{ 0, 0.f, 0.f}{}
    constexpr RangeHint(const LADSPA_PortRangeHint &_hint)
    : LADSPA_PortRangeHint(_hint)
    {}
    constexpr RangeHint(const RangeHint&_hint) = default;
    constexpr RangeHint(RangeHint&&_hint) noexcept= default;
    RangeHint&operator=(const RangeHint&_hint) = default;
    RangeHint&operator=(RangeHint&&_hint) noexcept= default;

    constexpr RangeHint(LADSPA_PortRangeHint &&_hint) noexcept
    : LADSPA_PortRangeHint(_hint)
    {}
    bool boundedBelow() const { return LADSPA_IS_HINT_BOUNDED_BELOW(HintDescriptor);}
    bool boundedAbove() const { return LADSPA_IS_HINT_BOUNDED_ABOVE(HintDescriptor);}
    bool toggled() const { return LADSPA_IS_HINT_TOGGLED(HintDescriptor);}
    bool sampleRate() const { return LADSPA_IS_HINT_SAMPLE_RATE(HintDescriptor);}
    bool logarithmic() const { return LADSPA_IS_HINT_LOGARITHMIC(HintDescriptor);}
    bool integer() const { return LADSPA_IS_HINT_INTEGER(HintDescriptor);}
    float lowerBound() const { return boundedBelow() ? LowerBound : -std::numeric_limits<float>::infinity();}
    float upperBound() const { return boundedAbove() ? UpperBound :  std::numeric_limits<float>::infinity();}
    void setLowerBound(float x)
    { if(std::isfinite(x)) { setBoundedBelow(true); LowerBound = x;} else { setBoundedBelow(false); LowerBound = x;} }
    void setUpperBound(float x)
    { if(std::isfinite(x)) { setBoundedAbove(true); UpperBound = x;} else { setBoundedAbove(false); UpperBound = x;} }

    void setBoundedBelow(bool x) { HintDescriptor = int(bitmask_set(HintDescriptor,LADSPA_HINT_BOUNDED_BELOW, x));}
    void setBoundedAbove(bool x) { HintDescriptor = int(bitmask_set(HintDescriptor,LADSPA_HINT_BOUNDED_ABOVE, x));}
    void setToggled(bool x) { HintDescriptor = int(bitmask_set(HintDescriptor,LADSPA_HINT_TOGGLED, x));}
    void setSampleRate(bool x) { HintDescriptor = int(bitmask_set(HintDescriptor,LADSPA_HINT_SAMPLE_RATE, x));}
    void setLogarithmic(bool x) { HintDescriptor = int(bitmask_set(HintDescriptor,LADSPA_HINT_LOGARITHMIC, x));}
    void setInteger(bool x) { HintDescriptor = int(bitmask_set(HintDescriptor,LADSPA_HINT_INTEGER, x));}
    DefaultType defaultType() const;
    void setDefaultType(DefaultType x);
    float defaultValue() const;
};
QML_DECLARE_TYPE(RangeHint)

class LadspaPort : public QObject {
    Q_OBJECT
protected:
    const LADSPA_Descriptor *m_descriptor{};
    LADSPA_Handle            m_handle{};
    unsigned long            m_port_id{};
    float                   *m_connection{};
private:
    Q_PROPERTY(bool input READ isInput)
    Q_PROPERTY(bool output READ isOutput)
    Q_PROPERTY(bool audio READ isAudio)
    Q_PROPERTY(bool control READ isControl)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(RangeHint rangeHint READ rangeHint)
    Q_PROPERTY(float * connection READ connection WRITE connectPort)
public:
    LadspaPort(const LADSPA_Descriptor  *_d, unsigned long _id, LADSPA_Handle _h = nullptr, QObject *pParent = nullptr);
    virtual ~LadspaPort();
    LADSPA_PortDescriptor portDesc() const;
    RangeHint rangeHint() const ;
    QString   name() const ;
    bool isInput() const ;
    bool isOutput() const ;
    bool isAudio() const ;
    bool isControl() const ;
    float *connection() const ;
    void connectPort(float *_conn);
    void setHandle(LADSPA_Handle _h);
};

QML_DECLARE_TYPE(LadspaPort)
