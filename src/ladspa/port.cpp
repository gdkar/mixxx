
#include <QMetaType>
#include <QMetaObject>
#include <QtQml>
#include <QtQuick>
#include <QtGlobal>

#include "ladspa/port.hpp"
#include "ladspa/descriptor.hpp"
#include "ladspa/effect.hpp"

LadspaPort::~LadspaPort() = default;
LadspaPort::LadspaPort(const LADSPA_Descriptor  *_d, unsigned long _id, LADSPA_Handle _h , QObject *pParent )
: QObject(pParent)
, m_descriptor(_d)
, m_handle(_h)
, m_port_id(_id)
{
    if(_d)
        setObjectName(name());
}
void LadspaPort::connectPort(float *_conn)
{
    if(m_descriptor->connect_port
    && m_handle) {
        m_descriptor->connect_port(m_handle, m_port_id, _conn);
        m_connection = _conn;
    }
}
LADSPA_PortDescriptor LadspaPort::portDesc() const { return m_descriptor ? m_descriptor->PortDescriptors[m_port_id] : 0;}
RangeHint LadspaPort::rangeHint() const { return RangeHint{m_descriptor->PortRangeHints[m_port_id]};}
QString   LadspaPort::name() const { return m_descriptor->PortNames[m_port_id];}
bool LadspaPort::isInput() const { return LADSPA_IS_PORT_INPUT(portDesc());}
bool LadspaPort::isOutput() const { return LADSPA_IS_PORT_OUTPUT(portDesc());}
bool LadspaPort::isAudio() const { return LADSPA_IS_PORT_AUDIO(portDesc());}
bool LadspaPort::isControl() const { return LADSPA_IS_PORT_CONTROL(portDesc());}
float *LadspaPort::connection() const { return m_connection;}
void LadspaPort::setHandle(LADSPA_Handle _h)
{
    m_handle = _h;
}

RangeHint::DefaultType RangeHint::defaultType() const { return DefaultType(HintDescriptor & int(DefaultType::Mask));}
void RangeHint::setDefaultType(DefaultType x)
{
    HintDescriptor &= ~int(DefaultType::Mask);
    HintDescriptor |= (int(DefaultType::Mask)|int(x));
}
float RangeHint::defaultValue() const
{
    auto && lerp = [](float x, float y, float m) { return (y * m) + x * ( 1.f - m);};
    auto && mix  = [&](float m)
    {
        return logarithmic()
        ? std::exp(lerp(std::log(lowerBound()),std::log(upperBound()),m))
        : lerp(lowerBound(),upperBound(),m);
    };
    switch(defaultType()) {
        case DefaultType::None: return 0.f;
        case DefaultType::Minimum: return lowerBound();
        case DefaultType::Low: return mix(0.25f);
        case DefaultType::Middle: return mix(0.5f);
        case DefaultType::High: return mix(0.75f);
        case DefaultType::Maximum: return upperBound();
        case DefaultType::Zero: return 0.f;
        case DefaultType::One: return 1.f;
        case DefaultType::OneHundred: return 100.f;
        case DefaultType::FourForty: return 440.f;
        default:                    return 0.f;
    }
}
