#include <QtDebug>

#include "controleffectknob.h"
#include "effects/effectbuttonparameterslot.h"
#include "controlobject.h"
#include "controlpushbutton.h"
#include "util/math.h"

EffectButtonParameterSlot::EffectButtonParameterSlot(QString group,const unsigned int iParameterSlotNumber)
        : EffectParameterSlotBase(group,iParameterSlotNumber)
{
    QString itemPrefix = formatItemPrefix(iParameterSlotNumber);
    m_pControlLoaded = new ControlObject(ConfigKey(m_group, itemPrefix + QString("_loaded")));
    m_pControlValue = new ControlPushButton(ConfigKey(m_group, itemPrefix));
    m_pControlValue->setProperty("buttonMode",ControlPushButton::POWERWINDOW);
    m_pControlType = new ControlObject(ConfigKey(m_group, itemPrefix + QString("_type")));
    connect(m_pControlValue, SIGNAL(valueChanged(double)),this, SLOT(slotValueChanged(double)));
    // Read-only controls.
    m_pControlType->connectValueChangeRequest(this, SLOT(slotValueType(double)));
    m_pControlLoaded->connectValueChangeRequest(this, SLOT(slotLoaded(double)));
    clear();
}
EffectButtonParameterSlot::~EffectButtonParameterSlot()
{
    //qDebug() << debugString() << "destroyed";
    delete m_pControlValue;
}

void EffectButtonParameterSlot::loadEffect(EffectPointer pEffect) {
    //qDebug() << debugString() << "loadEffect" << (pEffect ? pEffect->getManifest().name() : "(null)");
    if (m_pEffectParameter) {
        clear();
    }
    if (pEffect)
    {
        m_pEffect = pEffect;
        // Returns null if it doesn't have a parameter for that number
        m_pEffectParameter = pEffect->getButtonParameterForSlot(m_iParameterSlotNumber);
        if (m_pEffectParameter)
        {
            // Set the number of states
            auto numStates = math_max(m_pEffectParameter->manifest().getSteps().size(), 2);
            m_pControlValue->setStates(numStates);
            //qDebug() << debugString() << "Loading effect parameter" << m_pEffectParameter->name();
            auto  dValue = m_pEffectParameter->getValue();
            auto dMinimum = m_pEffectParameter->getMinimum();
            auto dMinimumLimit = dMinimum; // TODO(rryan) expose limit from EffectParameter
            auto dMaximum = m_pEffectParameter->getMaximum();
            auto  dMaximumLimit = dMaximum; // TODO(rryan) expose limit from EffectParameter
            auto dDefault = m_pEffectParameter->getDefault();
            if (dValue > dMaximum || dValue < dMinimum ||
                dMinimum < dMinimumLimit || dMaximum > dMaximumLimit ||
                dDefault > dMaximum || dDefault < dMinimum)
            {
                qWarning() << debugString() << "WARNING: EffectParameter does not satisfy basic sanity checks.";
            }
            m_pControlValue->set(dValue);
            m_pControlValue->setDefaultValue(dDefault);
            EffectManifestParameter::ControlHint type = m_pEffectParameter->getControlHint();
            // TODO(rryan) expose this from EffectParameter
            m_pControlType->setAndConfirm(static_cast<double>(type));
            // Default loaded parameters to loaded and unlinked
            m_pControlLoaded->setAndConfirm(1.0);
            connect(m_pEffectParameter, SIGNAL(valueChanged(double)),this, SLOT(slotParameterValueChanged(double)));
        }
    }
    emit(updated());
}
void EffectButtonParameterSlot::clear() {
    //qDebug() << debugString() << "clear";
    if (m_pEffectParameter)
    {
        m_pEffectParameter->disconnect(this);
        m_pEffectParameter = nullptr;
    }
    m_pEffect.clear();
    m_pControlLoaded->setAndConfirm(0.0);
    m_pControlValue->set(0.0);
    m_pControlValue->setDefaultValue(0.0);
    m_pControlType->setAndConfirm(0.0);
    emit(updated());
}
void EffectButtonParameterSlot::slotParameterValueChanged(double value)
{
    //qDebug() << debugString() << "slotParameterValueChanged" << value.toDouble();
    m_pControlValue->set(value);
}
void EffectButtonParameterSlot::slotValueChanged(double v)
{
    if (m_pEffectParameter) m_pEffectParameter->setValue(v);
}
