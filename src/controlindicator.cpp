#include "controlindicator.h"
#include "util/math.h"


/* static */
std::atomic<int> ControlIndicator::s_tick{0};
QTimer           ControlIndicator::s_timer{};
/* static */
void ControlIndicator::initialize()
{
  s_timer.setTimerType(Qt::CoarseTimer);
  s_timer.setInterval(250);
  s_timer.start();
  QObject::connect(&s_timer,&QTimer::timeout,[](){ ControlIndicator::s_tick++; });
}
ControlIndicator::ControlIndicator(ConfigKey key)
        : ControlObject(key, false),
          m_blinkValue(BlinkValue::Off)
{
    connect(this, &ControlIndicator::blinkValueChanged, this, &ControlIndicator::onTick);
    connect(&s_timer,&QTimer::timeout,this,&ControlIndicator::onTick);
}
ControlIndicator::~ControlIndicator() = default;
void ControlIndicator::setBlinkValue(BlinkValue bv)
{
    if (m_blinkValue.exchange(bv) != bv) emit(blinkValueChanged());
}
void ControlIndicator::onTick()
{
    auto ticks = s_tick.load();
    switch ( m_blinkValue.load() )
    {
      case BlinkValue::Off:
        set(0.0);
        break;
      case BlinkValue::On:
        set(1.0);
        break;
      case BlinkValue::Fast:
        set( (ticks & 1) ? 1.0 : 0.0);
        break;
      case BlinkValue::Slow:
        set( (ticks & 2) ? 1.0 : 0.0);
        break;
      default:
        break;
    }
}
void ControlIndicator::set(double v)
{
  ControlObject::set(v);
}
ControlIndicator::BlinkValue ControlIndicator::blinkValue() const
{
  return m_blinkValue.load();
}
