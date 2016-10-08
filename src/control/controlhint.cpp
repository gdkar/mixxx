#include "control/controlhint.h"
#include <cmath>
#include <limits>
#include <numeric>
double ControlHint::lowerBound() const
{
    return boundedBelow() ? m_lower : -std::numeric_limits<double>::infinity();
}
void ControlHint::setLowerBound(double val)
{
    if(std::isfinite(val)) {
        m_desc |= LADSPA_HINT_BOUNDED_BELOW;
        m_lower = val;
    }else{
        m_desc &= ~LADSPA_HINT_BOUNDED_BELOW;
        m_lower = -std::numeric_limits<double>::infinity();
    }
}
double ControlHint::upperBound() const
{
    return boundedAbove() ? m_upper : std::numeric_limits<double>::infinity();
}
void ControlHint::setUpperBound(double val)
{
    if(std::isfinite(val)) {
        m_desc |= LADSPA_HINT_BOUNDED_ABOVE;
        m_upper = val;
    }else{
        m_desc &= ~LADSPA_HINT_BOUNDED_ABOVE;
        m_upper = std::numeric_limits<double>::infinity();
    }
}
bool ControlHint::boundedBelow() const
{
    return LADSPA_IS_HINT_BOUNDED_BELOW(m_desc);
}
bool ControlHint::boundedAbove() const
{
    return LADSPA_IS_HINT_BOUNDED_ABOVE(m_desc);
}
bool ControlHint::toggled() const
{
    return LADSPA_IS_HINT_TOGGLED(m_desc);
}
bool ControlHint::sampleRate() const
{
    return LADSPA_IS_HINT_SAMPLE_RATE(m_desc);
}
bool ControlHint::logarithmic() const
{
    return LADSPA_IS_HINT_LOGARITHMIC(m_desc);
}
bool ControlHint::integer() const
{
    return LADSPA_IS_HINT_INTEGER(m_desc);
}
bool ControlHint::hasDefault() const
{
    return LADSPA_IS_HINT_HAS_DEFAULT(m_desc)
       && defaultType() != Default::None;
}

void ControlHint::setToggled(bool val)
{
    if(val)
        m_desc |= LADSPA_HINT_TOGGLED;
    else
        m_desc &= ~LADSPA_HINT_TOGGLED;
}
void ControlHint::setSampleRate(bool val)
{
    if(val)
        m_desc |= LADSPA_HINT_SAMPLE_RATE;
    else
        m_desc &= ~LADSPA_HINT_SAMPLE_RATE;
}
void ControlHint::setLogarithmic(bool val)
{
    if(val)
        m_desc |= LADSPA_HINT_LOGARITHMIC;
    else
        m_desc &= ~LADSPA_HINT_LOGARITHMIC;
}
void ControlHint::setInteger(bool val)
{
    if(val)
        m_desc |= LADSPA_HINT_INTEGER;
    else
        m_desc &= ~LADSPA_HINT_INTEGER;
}
ControlHint::Default ControlHint::defaultType() const
{
    return Default(m_desc & LADSPA_HINT_DEFAULT_MASK);
}
void ControlHint::setDefaultType(Default type)
{
    m_desc &= ~LADSPA_HINT_DEFAULT_MASK;
    m_desc |= (LADSPA_HINT_DEFAULT_MASK & int(type));
}
double ControlHint::defaultValue() const
{
    if(!hasDefault())
        return 0.;
    auto proportion = 0.5;
    switch(defaultType()) {
        case Default::Minimum: proportion = 0.;break;
        case Default::Low: proportion = 0.25;break;
        case Default::Middle: proportion = 0.5; break;
        case Default::High: proportion = 0.75;break;
        case Default::Maximum: proportion = 1.; break;
        case Default::Zero: return 0.;
        case Default::One:  return 1.;
        case Default::Hundred: return 100.;
        case Default::FourForty: return 440.;
        default:                 return 0.;
    }
    auto val = (logarithmic()) ? std::exp(std::log(lowerBound()) * (1.-proportion)
                                           +std::log(upperBound()) * proportion)
                                 : lowerBound() * (1.-proportion)
                                  +upperBound() * proportion;
    if(integer())
        val = std::rint(val);
    return val;
}

double ControlHint::proportionToValue(double proportion) const
{
    if(toggled())
        return proportion > 0;
    else if(!boundedAbove() || !boundedBelow()) {
        return integer() ? rint(proportion) : proportion;
    }
    auto val = (logarithmic()) ? std::exp(std::log(lowerBound()) * (1.-proportion)
                                           +std::log(upperBound()) * (proportion))
                                 : lowerBound() * (1.-proportion)
                                  +upperBound() * (proportion   );
    if(integer())
        val = std::rint(val);

    return val;
}
double ControlHint::valueToProportion(double val) const
{
    if(toggled())
        return val ? 1. : 0.;

    else if(!boundedAbove() || !boundedBelow()) {
        val = logarithmic() ? std::log(val) : val;
        return integer() ? rint(val) : val;
    }
    if(integer())
        val = std::rint(val);
    auto proportion = (logarithmic())
      ? ((std::log(val) - std::log(lowerBound()))/(std::log(upperBound()) - std::log(lowerBound())))
      : (val - lowerBound()) / (upperBound() - lowerBound());

    return proportion;
}
