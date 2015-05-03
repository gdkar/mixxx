#include "effects/effectslot.h"

#include <QDebug>

#include "control/controlpushbutton.h"
#include "control/controlobjectslave.h"

// The maximum number of effect parameters we're going to support.
const unsigned int kDefaultMaxParameters = 16;

EffectSlot::EffectSlot(const QString& group,
                       const unsigned int iChainNumber,
                       const unsigned int iEffectnumber)
        : m_iChainNumber(iChainNumber),
          m_iEffectNumber(iEffectnumber),
          m_group(group) {
    m_pControlLoaded = new ControlObject(ConfigKey(m_group, "loaded"));
    m_pControlLoaded->connectValueChangeRequest(
        this, SLOT(onLoaded(double)));

    m_pControlNumParameters = new ControlObject(ConfigKey(m_group, "num_parameters"));
    m_pControlNumParameters->connectValueChangeRequest(
        this, SLOT(onNumParameters(double)));

    m_pControlNumParameterSlots = new ControlObject(ConfigKey(m_group, "num_parameterslots"));
    m_pControlNumParameterSlots->connectValueChangeRequest(
        this, SLOT(onNumParameterSlots(double)));

    m_pControlNumButtonParameters = new ControlObject(ConfigKey(m_group, "num_button_parameters"));
    m_pControlNumButtonParameters->connectValueChangeRequest(
        this, SLOT(onNumParameters(double)));

    m_pControlNumButtonParameterSlots = new ControlObject(ConfigKey(m_group, "num_button_parameterslots"));
    m_pControlNumButtonParameterSlots->connectValueChangeRequest(
        this, SLOT(onNumParameterSlots(double)));

    m_pControlEnabled = new ControlPushButton(ConfigKey(m_group, "enabled"));
    m_pControlEnabled->setButtonMode(ControlPushButton::POWERWINDOW);
    // Default to enabled. The skin might not show these buttons.
    m_pControlEnabled->setDefaultValue(true);
    m_pControlEnabled->set(true);
    connect(m_pControlEnabled, SIGNAL(valueChanged(double)),
            this, SLOT(onEnabled(double)));

    m_pControlNextEffect = new ControlPushButton(ConfigKey(m_group, "next_effect"));
    connect(m_pControlNextEffect, SIGNAL(valueChanged(double)),
            this, SLOT(onNextEffect(double)));

    m_pControlPrevEffect = new ControlPushButton(ConfigKey(m_group, "prev_effect"));
    connect(m_pControlPrevEffect, SIGNAL(valueChanged(double)),
            this, SLOT(onPrevEffect(double)));

    // Ignoring no-ops is important since this is for +/- tickers.
    m_pControlEffectSelector = new ControlObject(ConfigKey(m_group, "effect_selector"), false);
    connect(m_pControlEffectSelector, SIGNAL(valueChanged(double)),
            this, SLOT(onEffectSelector(double)));

    m_pControlClear = new ControlPushButton(ConfigKey(m_group, "clear"));
    connect(m_pControlClear, SIGNAL(valueChanged(double)),
            this, SLOT(onClear(double)));

    for (unsigned int i = 0; i < kDefaultMaxParameters; ++i) {
        addEffectParameterSlot();
        addEffectButtonParameterSlot();
    }

    clear();
}

EffectSlot::~EffectSlot() {
    //qDebug() << debugString() << "destroyed";
    clear();

    delete m_pControlLoaded;
    delete m_pControlNumParameters;
    delete m_pControlNumParameterSlots;
    delete m_pControlNumButtonParameters;
    delete m_pControlNumButtonParameterSlots;
    delete m_pControlNextEffect;
    delete m_pControlPrevEffect;
    delete m_pControlEffectSelector;
    delete m_pControlClear;
    delete m_pControlEnabled;
}

EffectParameterSlotPointer EffectSlot::addEffectParameterSlot() {
    EffectParameterSlotPointer pParameter = EffectParameterSlotPointer(
            new EffectParameterSlot(m_group, m_parameters.size()));
    m_parameters.append(pParameter);
    m_pControlNumParameterSlots->setAndConfirm(
            m_pControlNumParameterSlots->get() + 1);
    return pParameter;
}

EffectButtonParameterSlotPointer EffectSlot::addEffectButtonParameterSlot() {
    EffectButtonParameterSlotPointer pParameter = EffectButtonParameterSlotPointer(
            new EffectButtonParameterSlot(m_group, m_buttonParameters.size()));
    m_buttonParameters.append(pParameter);
    m_pControlNumButtonParameterSlots->setAndConfirm(
            m_pControlNumButtonParameterSlots->get() + 1);
    return pParameter;
}

EffectPointer EffectSlot::getEffect() const {
    return m_pEffect;
}

unsigned int EffectSlot::numParameterSlots() const {
    return m_parameters.size();
}

unsigned int EffectSlot::numButtonParameterSlots() const {
    return m_buttonParameters.size();
}

void EffectSlot::onLoaded(double v) {
    Q_UNUSED(v);
    //qDebug() << debugString() << "onLoaded" << v;
    qWarning() << "WARNING: loaded is a read-only control.";
}

void EffectSlot::onNumParameters(double v) {
    Q_UNUSED(v);
    //qDebug() << debugString() << "onNumParameters" << v;
    qWarning() << "WARNING: num_parameters is a read-only control.";
}

void EffectSlot::onNumParameterSlots(double v) {
    Q_UNUSED(v);
    //qDebug() << debugString() << "onNumParameterSlots" << v;
    qWarning() << "WARNING: num_parameterslots is a read-only control.";
}

void EffectSlot::onEnabled(double v) {
    //qDebug() << debugString() << "onEnabled" << v;
    if (m_pEffect) {
        m_pEffect->setEnabled(v > 0);
    }
}

void EffectSlot::onEffectEnabledChanged(bool enabled) {
    m_pControlEnabled->set(enabled);
}

EffectParameterSlotPointer EffectSlot::getEffectParameterSlot(unsigned int onNumber) {
    //qDebug() << debugString() << "getEffectParameterSlot" << onNumber;
    if (onNumber >= static_cast<unsigned int>(m_parameters.size())) {
        qWarning() << "WARNING: onNumber out of range";
        return EffectParameterSlotPointer();
    }
    return m_parameters[onNumber];
}

EffectButtonParameterSlotPointer EffectSlot::getEffectButtonParameterSlot(unsigned int onNumber) {
    //qDebug() << debugString() << "getEffectParameterSlot" << onNumber;
    if (onNumber >= static_cast<unsigned int>(m_buttonParameters.size())) {
        qWarning() << "WARNING: onNumber out of range";
        return EffectButtonParameterSlotPointer();
    }
    return m_buttonParameters[onNumber];
}

void EffectSlot::loadEffect(EffectPointer pEffect) {
    //qDebug() << debugString() << "loadEffect"
    //         << (pEffect ? pEffect->getManifest().name() : "(null)");
    if (pEffect) {
        m_pEffect = pEffect;
        m_pControlLoaded->setAndConfirm(1.0);
        m_pControlNumParameters->setAndConfirm(pEffect->numKnobParameters());
        m_pControlNumButtonParameters->setAndConfirm(pEffect->numButtonParameters());

        // Enabled is a persistent property of the effect slot, not of the
        // effect. Propagate the current setting to the effect.
        pEffect->setEnabled(m_pControlEnabled->get() > 0.0);

        connect(pEffect.data(), SIGNAL(enabledChanged(bool)),
                this, SLOT(onEffectEnabledChanged(bool)));

        while (static_cast<unsigned int>(m_parameters.size()) < pEffect->numKnobParameters()) {
            addEffectParameterSlot();
        }

        while (static_cast<unsigned int>(m_buttonParameters.size()) < pEffect->numButtonParameters()) {
            addEffectButtonParameterSlot();
        }

        foreach (EffectParameterSlotPointer pParameter, m_parameters) {
            pParameter->loadEffect(pEffect);
        }

        foreach (EffectButtonParameterSlotPointer pParameter, m_buttonParameters) {
            pParameter->loadEffect(pEffect);
        }

        emit(effectLoaded(pEffect, m_iEffectNumber));
    } else {
        clear();
        // Broadcasts a null effect pointer
        emit(effectLoaded(EffectPointer(), m_iEffectNumber));
    }
    emit(updated());
}

void EffectSlot::clear() {
    if (m_pEffect) {
        m_pEffect->disconnect(this);
    }
    m_pControlLoaded->setAndConfirm(0.0);
    m_pControlNumParameters->setAndConfirm(0.0);
    m_pControlNumButtonParameters->setAndConfirm(0.0);
    foreach (EffectParameterSlotPointer pParameter, m_parameters) {
        pParameter->clear();
    }
    foreach (EffectButtonParameterSlotPointer pParameter, m_buttonParameters) {
        pParameter->clear();
    }
    m_pEffect.clear();
    emit(updated());
}

void EffectSlot::onPrevEffect(double v) {
    if (v > 0) {
        onEffectSelector(-1);
    }
}

void EffectSlot::onNextEffect(double v) {
    if (v > 0) {
        onEffectSelector(1);
    }
}

void EffectSlot::onEffectSelector(double v) {
    if (v > 0) {
        emit(nextEffect(m_iChainNumber, m_iEffectNumber, m_pEffect));
    } else if (v < 0) {
        emit(prevEffect(m_iChainNumber, m_iEffectNumber, m_pEffect));
    }
}

void EffectSlot::onClear(double v) {
    if (v > 0) {
        emit(clearEffect(m_iEffectNumber));
    }
}

void EffectSlot::onChainSuperParameterChanged(double parameter, bool force) {
    for (int i = 0; i < m_parameters.size(); ++i) {
        m_parameters[i]->onChainSuperParameterChanged(parameter, force);
    }
}

void EffectSlot::syncSofttakeover() {
    for (int i = 0; i < m_parameters.size(); ++i) {
        m_parameters[i]->syncSofttakeover();
    }
}
