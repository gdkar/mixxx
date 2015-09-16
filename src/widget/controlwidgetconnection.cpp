#include "widget/controlwidgetconnection.h"

#include "widget/wbasewidget.h"
#include "controlobjectslave.h"
#include "util/debug.h"
#include "util/assert.h"

ControlWidgetConnection::ControlWidgetConnection(WBaseWidget* pBaseWidget,ControlObjectSlave* pControl)
        : m_pWidget(pBaseWidget),
          m_pControl(pControl){
    // If m_pControl is nullptr then the creator of ControlWidgetConnection has
    // screwed up badly. Assert in development mode. In release mode the
    // connection will be defunct.
    DEBUG_ASSERT_AND_HANDLE(!m_pControl.isNull()) {m_pControl.reset(new ControlObjectSlave());}
    m_pControl->connectValueChanged(this, SLOT(slotControlValueChanged(double)));
}

void ControlWidgetConnection::setInvert(bool i){m_bInvert=i;}
bool ControlWidgetConnection::invert()const{return m_bInvert;}
ControlWidgetConnection::~ControlWidgetConnection() = default;
const ConfigKey& ControlWidgetConnection::getKey()const{return m_pControl->getKey();}
void ControlWidgetConnection::setControlParameter(double parameter) {
    m_pControl->setParameter((invert()?static_cast<double>(!parameter):parameter));
}
int ControlParameterWidgetConnection::getDirectionOption()const{return m_directionOption;}
int ControlParameterWidgetConnection::getEmitOption()const{return m_emitOption;}
void ControlParameterWidgetConnection::setDirectionOption(
    enum ControlParameterWidgetConnection::DirectionOption v)
{
  m_directionOption = v;
}
void ControlParameterWidgetConnection::setEmitOption(
    enum ControlParameterWidgetConnection::EmitOption v )
{
  m_emitOption = v;
}
/* static */
QString ControlParameterWidgetConnection::emitOptionToString(
    ControlParameterWidgetConnection::EmitOption option)
{
        switch (option & EMIT_ON_PRESS_AND_RELEASE) {
            case EMIT_NEVER:                return "NEVER";
            case EMIT_ON_PRESS:             return "PRESS";
            case EMIT_ON_RELEASE:           return "RELEASE";
            case EMIT_ON_PRESS_AND_RELEASE: return "PRESS_AND_RELEASE";
            default:                        return "UNKNOWN";
        }
}
/*static*/ QString ControlParameterWidgetConnection::directionOptionToString(
    ControlParameterWidgetConnection::DirectionOption option) {
    switch (option & DIR_FROM_AND_TO_WIDGET) {
        case DIR_NON:                   return "NON";
        case DIR_FROM_WIDGET:           return "FROM_WIDGET";
        case DIR_TO_WIDGET:             return "TO_WIDGET";
        case DIR_FROM_AND_TO_WIDGET:    return "FROM_AND_TO_WIDGET";
        default:                        return "UNKNOWN";
    }
}
double ControlWidgetConnection::getControlParameter() const {
    double parameter = m_pControl->getParameter();
    if (invert()) {parameter = !parameter;}
    return (invert()?static_cast<double>(!parameter):parameter);
}
double ControlWidgetConnection::getControlParameterForValue(double value) const {
    double parameter = m_pControl->getParameterForValue(value);
    return (invert()?static_cast<double>(!parameter):parameter);
}
ControlParameterWidgetConnection::ControlParameterWidgetConnection(WBaseWidget* pBaseWidget,
                                                                   ControlObjectSlave* pControl, 
                                                                   DirectionOption directionOption,
                                                                   EmitOption emitOption)
        : ControlWidgetConnection(pBaseWidget, pControl),
          m_directionOption(directionOption),
          m_emitOption(emitOption) {}
ControlParameterWidgetConnection::~ControlParameterWidgetConnection() = default;
void ControlParameterWidgetConnection::Init() {
  if(m_pControl) slotControlValueChanged(m_pControl->get());
}
QString ControlParameterWidgetConnection::toDebugString() const {
    const ConfigKey& key = getKey();
    return QString("%1,%2 Parameter: %3 Direction: %4 Emit: %5")
            .arg(key.group, key.item,
                 QString::number(m_pControl->getParameter()),
                 directionOptionToString(m_directionOption),
                 emitOptionToString(m_emitOption));
}

void ControlParameterWidgetConnection::slotControlValueChanged(double value) {
    if (m_directionOption & DIR_TO_WIDGET) {
        double parameter = getControlParameterForValue(value);
        m_pWidget->onConnectedControlChanged(parameter, value);
    }
}

void ControlParameterWidgetConnection::resetControl() {
    if (m_directionOption & DIR_FROM_WIDGET) {
        m_pControl->reset();
    }
}

void ControlParameterWidgetConnection::setControlParameter(double v) {
    if (m_directionOption & DIR_FROM_WIDGET) {
        ControlWidgetConnection::setControlParameter(v);
    }
}

void ControlParameterWidgetConnection::setControlParameterDown(double v) {
    if ((m_directionOption & DIR_FROM_WIDGET) && (m_emitOption & EMIT_ON_PRESS)) {
        ControlWidgetConnection::setControlParameter(v);
    }
}

void ControlParameterWidgetConnection::setControlParameterUp(double v) {
    if ((m_directionOption & DIR_FROM_WIDGET) && (m_emitOption & EMIT_ON_RELEASE)) {
        ControlWidgetConnection::setControlParameter(v);
    }
}

ControlWidgetPropertyConnection::ControlWidgetPropertyConnection(WBaseWidget* pBaseWidget,
                                                                 ControlObjectSlave* pControl, 
                                                                 const QString& propertyName)
        : ControlWidgetConnection(pBaseWidget, pControl),
          m_propertyName(propertyName.toAscii()) {
    slotControlValueChanged(m_pControl->get());
}

ControlWidgetPropertyConnection::~ControlWidgetPropertyConnection() = default;

QString ControlWidgetPropertyConnection::toDebugString() const {
    const ConfigKey& key = getKey();
    return QString("%1,%2 Parameter: %3 Property: %4 Value: %5").arg(
        key.group, key.item, QString::number(m_pControl->getParameter()), m_propertyName,
        m_pWidget->toQWidget()->property(
            m_propertyName.constData()).toString());
}

void ControlWidgetPropertyConnection::slotControlValueChanged(double v) {
    QVariant parameter;
    QWidget* pWidget = m_pWidget->toQWidget();
    QVariant property = pWidget->property(m_propertyName.constData());
    if (property.type() == QVariant::Bool) {
        parameter = getControlParameterForValue(v) > 0;
    } else {
        parameter = getControlParameterForValue(v);
    }

    if (!pWidget->setProperty(m_propertyName.constData(),parameter)) {
        qDebug() << "Setting property" << m_propertyName
                << "to widget failed. Value:" << parameter;
    }
}
