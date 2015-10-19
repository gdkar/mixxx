#include <QMetaEnum>
#include <QString>
#include "widget/controlwidgetconnection.h"

#include "widget/wbasewidget.h"
#include "controlobject.h"
#include "util/debug.h"
#include "util/assert.h"

ControlWidgetConnection::ControlWidgetConnection(WBaseWidget* pBaseWidget,ControlObject* pControl)
        : m_pWidget(pBaseWidget),
          m_pControl(pControl){
    // If m_pControl is nullptr then the creator of ControlWidgetConnection has
    // screwed up badly. Assert in development mode. In release mode the
    // connection will be defunct.
    DEBUG_ASSERT_AND_HANDLE(!m_pControl.isNull()) m_pControl.reset(new ControlObject());
    m_pControl->connectValueChanged(this, SLOT(slotControlValueChanged(double)));
    connect(m_pControl.data(),&ControlObject::valueChanged,this,&ControlWidgetConnection::controlParameterChanged,
        static_cast<Qt::ConnectionType>(Qt::DirectConnection|Qt::UniqueConnection));
}

void ControlWidgetConnection::setInvert(bool i)
{
  if ( m_bInvert != i )
  {
    m_bInvert=i;
    emit invertChanged(i);
    if(m_pControl) slotControlValueChanged(m_pControl->get());
  }
}
bool ControlWidgetConnection::invert()const
{
  return m_bInvert;
}
ControlWidgetConnection::~ControlWidgetConnection() = default;
const ConfigKey& ControlWidgetConnection::getKey()const
{
  return m_pControl->getKey();
}
void ControlWidgetConnection::setControlParameter(double parameter)
{
    m_pControl->setParameter((invert()?static_cast<double>(!parameter):parameter));
}
ControlParameterWidgetConnection::DirectionOptions ControlParameterWidgetConnection::getDirectionOption()const
{
  return m_directionOption;
}
ControlParameterWidgetConnection::EmitOptions ControlParameterWidgetConnection::getEmitOption()const
{
  return m_emitOption;
}
void ControlParameterWidgetConnection::setDirectionOption(ControlParameterWidgetConnection::DirectionOptions v)
{
  if ( m_directionOption != v)
  {
    m_directionOption = v;
    emit directionOptionChanged(v);
  }
}
void ControlParameterWidgetConnection::setEmitOption(ControlParameterWidgetConnection::EmitOptions v )
{
  if ( m_emitOption != v)
  {
    m_emitOption = v;
    emit emitOptionChanged(v);
  }
}
/* static */
QString ControlParameterWidgetConnection::emitOptionToString(EmitOptions option)
{
    auto emitEnum = QMetaEnum::fromType<EmitOption>();
    return QString{emitEnum.valueToKeys(static_cast<int>(option))};
}
/*static*/ QString ControlParameterWidgetConnection::directionOptionToString(DirectionOptions option)
{
    auto dirEnum = QMetaEnum::fromType<DirectionOption>();
    return QString{dirEnum.valueToKeys(static_cast<int>(option))};
}
double ControlWidgetConnection::getControlParameter() const
{
    auto parameter = m_pControl->getParameter();
    if (invert()) {parameter = !parameter;}
    return (invert()?static_cast<double>(!parameter):parameter);
}
double ControlWidgetConnection::getControlParameterForValue(double value) const
{
    auto parameter = m_pControl->getParameterForValue(value);
    return (invert()?static_cast<double>(!parameter):parameter);
}
ControlParameterWidgetConnection::ControlParameterWidgetConnection(WBaseWidget* pBaseWidget,
                                                                   ControlObject* pControl, 
                                                                   DirectionOptions directionOption,
                                                                   EmitOptions emitOption)
        : ControlWidgetConnection(pBaseWidget, pControl),
          m_directionOption(directionOption),
          m_emitOption(emitOption)
{}
ControlParameterWidgetConnection::~ControlParameterWidgetConnection() = default;
void ControlParameterWidgetConnection::Init()
{
  if(m_pControl) slotControlValueChanged(m_pControl->get());
}
QString ControlParameterWidgetConnection::toDebugString() const
{
    auto key = getKey();
    return QString("%1,%2 Parameter: %3 Direction: %4 Emit: %5")
            .arg(key.group, key.item,
                 QString::number(m_pControl->getParameter()),
                 directionOptionToString(m_directionOption),
                 emitOptionToString(m_emitOption));
}
void ControlParameterWidgetConnection::slotControlValueChanged(double value)
{
    if (m_directionOption & DirectionOption::ToWidget) {
        auto parameter = getControlParameterForValue(value);
        m_pWidget->onConnectedControlChanged(parameter, value);
    }
}
void ControlParameterWidgetConnection::resetControl()
{
    if (m_directionOption & DirectionOption::FromWidget) m_pControl->reset();
}
void ControlParameterWidgetConnection::setControlParameter(double v)
{
    if (m_directionOption & DirectionOption::FromWidget) ControlWidgetConnection::setControlParameter(v);
}

void ControlParameterWidgetConnection::setControlParameterDown(double v)
{
    if ((m_directionOption & DirectionOption::FromWidget) && (m_emitOption & EmitOption::OnPress))
        ControlWidgetConnection::setControlParameter(v);
}
void ControlParameterWidgetConnection::setControlParameterUp(double v)
{
    if ((m_directionOption & DirectionOption::FromWidget) && (m_emitOption & EmitOption::OnRelease))
        ControlWidgetConnection::setControlParameter(v);
}
ControlWidgetPropertyConnection::ControlWidgetPropertyConnection(WBaseWidget* pBaseWidget,
                                                                 ControlObject* pControl, 
                                                                 const QString& propertyName)
        : ControlWidgetConnection(pBaseWidget, pControl),
          m_propertyName(propertyName.toAscii())
{
    slotControlValueChanged(m_pControl->get());
}
ControlWidgetPropertyConnection::~ControlWidgetPropertyConnection() = default;
QString ControlWidgetPropertyConnection::toDebugString() const
{
    auto key = getKey();
    return QString("%1,%2 Parameter: %3 Property: %4 Value: %5").arg(
        key.group, key.item, QString::number(m_pControl->getParameter()), m_propertyName,
        m_pWidget->toQWidget()->property( m_propertyName.constData()).toString());
}
void ControlWidgetPropertyConnection::slotControlValueChanged(double v)
{
    QVariant parameter;
    auto  pWidget = m_pWidget->toQWidget();
    auto property = pWidget->property(m_propertyName.constData());
    if (property.type() == QVariant::Bool) parameter = getControlParameterForValue(v) > 0;
    else                                   parameter = getControlParameterForValue(v);
    if (!pWidget->setProperty(m_propertyName.constData(),parameter)) {
        qDebug() << "Setting property" << m_propertyName << "to widget failed. Value:" << parameter;
    }
}
