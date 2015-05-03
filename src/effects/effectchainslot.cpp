#include "effects/effectchainslot.h"

#include "effects/effectrack.h"
#include "control/controlpotmeter.h"
#include "control/controlpushbutton.h"
#include "util/math.h"

EffectChainSlot::EffectChainSlot(EffectRack* pRack, const QString& group,
                                 unsigned int iChainNumber)
        : m_iChainSlotNumber(iChainNumber),
          // The control group names are 1-indexed while internally everything
          // is 0-indexed.
          m_group(group),
          m_pEffectRack(pRack) {
    m_pControlClear = new ControlPushButton(ConfigKey(m_group, "clear"));
    connect(m_pControlClear, SIGNAL(valueChanged(double)),
            this, SLOT(onControlClear(double)));

    m_pControlNumEffects = new ControlObject(ConfigKey(m_group, "num_effects"));
    m_pControlNumEffects->connectValueChangeRequest(
        this, SLOT(onControlNumEffects(double)));

    m_pControlNumEffectSlots = new ControlObject(ConfigKey(m_group, "num_effectslots"));
    m_pControlNumEffectSlots->connectValueChangeRequest(
        this, SLOT(onControlNumEffectSlots(double)));

    m_pControlChainLoaded = new ControlObject(ConfigKey(m_group, "loaded"));
    m_pControlChainLoaded->connectValueChangeRequest(
        this, SLOT(onControlChainLoaded(double)));

    m_pControlChainEnabled = new ControlPushButton(ConfigKey(m_group, "enabled"));
    m_pControlChainEnabled->setButtonMode(ControlPushButton::POWERWINDOW);
    // Default to enabled. The skin might not show these buttons.
    m_pControlChainEnabled->setDefaultValue(true);
    m_pControlChainEnabled->set(true);
    connect(m_pControlChainEnabled, SIGNAL(valueChanged(double)),
            this, SLOT(onControlChainEnabled(double)));

    m_pControlChainMix = new ControlPotmeter(ConfigKey(m_group, "mix"), 0.0, 1.0);
    connect(m_pControlChainMix, SIGNAL(valueChanged(double)),
            this, SLOT(onControlChainMix(double)));
    m_pControlChainMix->set(1.0);

    m_pControlChainSuperParameter = new ControlPotmeter(ConfigKey(m_group, "super1"), 0.0, 1.0);
    connect(m_pControlChainSuperParameter, SIGNAL(valueChanged(double)),
            this, SLOT(onControlChainSuperParameter(double)));
    m_pControlChainSuperParameter->set(0.0);
    m_pControlChainSuperParameter->setDefaultValue(0.0);

    m_pControlChainInsertionType = new ControlPushButton(ConfigKey(m_group, "insertion_type"));
    m_pControlChainInsertionType->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlChainInsertionType->setStates(EffectChain::NUM_INSERTION_TYPES);
    connect(m_pControlChainInsertionType, SIGNAL(valueChanged(double)),
            this, SLOT(onControlChainInsertionType(double)));

    m_pControlChainNextPreset = new ControlPushButton(ConfigKey(m_group, "next_chain"));
    connect(m_pControlChainNextPreset, SIGNAL(valueChanged(double)),
            this, SLOT(onControlChainNextPreset(double)));

    m_pControlChainPrevPreset = new ControlPushButton(ConfigKey(m_group, "prev_chain"));
    connect(m_pControlChainPrevPreset, SIGNAL(valueChanged(double)),
            this, SLOT(onControlChainPrevPreset(double)));

    // Ignoring no-ops is important since this is for +/- tickers.
    m_pControlChainSelector = new ControlObject(ConfigKey(m_group, "chain_selector"), false);
    connect(m_pControlChainSelector, SIGNAL(valueChanged(double)),
            this, SLOT(onControlChainSelector(double)));

    connect(&m_channelStatusMapper, SIGNAL(mapped(const QString&)),
            this, SLOT(onChannelStatusChanged(const QString&)));
}

EffectChainSlot::~EffectChainSlot() {
    //qDebug() << debugString() << "destroyed";
    clear();
    delete m_pControlClear;
    delete m_pControlNumEffects;
    delete m_pControlNumEffectSlots;
    delete m_pControlChainLoaded;
    delete m_pControlChainEnabled;
    delete m_pControlChainMix;
    delete m_pControlChainSuperParameter;
    delete m_pControlChainInsertionType;
    delete m_pControlChainPrevPreset;
    delete m_pControlChainNextPreset;
    delete m_pControlChainSelector;

    for (QMap<QString, ChannelInfo*>::iterator it = m_channelInfoByName.begin();
         it != m_channelInfoByName.end();) {
        delete it.value();
        it = m_channelInfoByName.erase(it);
    }

    m_slots.clear();
    m_pEffectChain.clear();
}

QString EffectChainSlot::id() const {
    if (m_pEffectChain)
        return m_pEffectChain->id();
    return "";
}

double EffectChainSlot::getSuperParameter() const {
    return m_pControlChainSuperParameter->get();
}

void EffectChainSlot::setSuperParameter(double value) {
    m_pControlChainSuperParameter->set(value);
}

void EffectChainSlot::setSuperParameterDefaultValue(double value) {
    m_pControlChainSuperParameter->setDefaultValue(value);
}

void EffectChainSlot::onChainNameChanged(const QString&) {
    emit(updated());
}

void EffectChainSlot::onChainEnabledChanged(bool bEnabled) {
    m_pControlChainEnabled->set(bEnabled);
    emit(updated());
}

void EffectChainSlot::onChainMixChanged(double mix) {
    m_pControlChainMix->set(mix);
    emit(updated());
}

void EffectChainSlot::onChainSuperParameterChanged(double parameter) {
    m_pControlChainSuperParameter->set(parameter);
    emit(updated());
}

void EffectChainSlot::onChainInsertionTypeChanged(EffectChain::InsertionType type) {
    m_pControlChainInsertionType->set(static_cast<double>(type));
    emit(updated());
}

void EffectChainSlot::onChainChannelStatusChanged(const QString& group,
                                                    bool enabled) {
    ChannelInfo* pInfo = m_channelInfoByName.value(group, NULL);
    if (pInfo != NULL && pInfo->pEnabled != NULL) {
        pInfo->pEnabled->set(enabled);
        emit(updated());
    }
}

void EffectChainSlot::onChainEffectsChanged(bool shouldEmit) {
    //qDebug() << debugString() << "onChainEffectsChanged";
    if (m_pEffectChain) {
        QList<EffectPointer> effects = m_pEffectChain->effects();
        if (effects.size() > m_slots.size()) {
            qWarning() << debugString() << "has too few slots for effect";
        }
        for (int i = 0; i < m_slots.size(); ++i) {
            EffectSlotPointer pSlot = m_slots[i];
            EffectPointer pEffect;
            if (i < effects.size()) {
                pEffect = effects[i];
            }
            if (pSlot)
                pSlot->loadEffect(pEffect);
        }
        m_pControlNumEffects->setAndConfirm(math_min(
            static_cast<unsigned int>(m_slots.size()),
            m_pEffectChain->numEffects()));
        if (shouldEmit) {
            emit(updated());
        }
    }
}

void EffectChainSlot::loadEffectChain(EffectChainPointer pEffectChain) {
    //qDebug() << debugString() << "loadEffectChain" << (pEffectChain ? pEffectChain->id() : "(null)");
    clear();

    if (pEffectChain) {
        m_pEffectChain = pEffectChain;
        m_pEffectChain->addToEngine(m_pEffectRack->getEngineEffectRack(),
                                    m_iChainSlotNumber);
        m_pEffectChain->updateEngineState();

        connect(m_pEffectChain.data(), SIGNAL(effectsChanged()),
                this, SLOT(onChainEffectsChanged()));
        connect(m_pEffectChain.data(), SIGNAL(nameChanged(const QString&)),
                this, SLOT(onChainNameChanged(const QString&)));
        connect(m_pEffectChain.data(), SIGNAL(enabledChanged(bool)),
                this, SLOT(onChainEnabledChanged(bool)));
        connect(m_pEffectChain.data(), SIGNAL(mixChanged(double)),
                this, SLOT(onChainMixChanged(double)));
        connect(m_pEffectChain.data(), SIGNAL(insertionTypeChanged(EffectChain::InsertionType)),
                this, SLOT(onChainInsertionTypeChanged(EffectChain::InsertionType)));
        connect(m_pEffectChain.data(), SIGNAL(channelStatusChanged(const QString&, bool)),
                this, SLOT(onChainChannelStatusChanged(const QString&, bool)));

        m_pControlChainLoaded->setAndConfirm(true);
        m_pControlChainInsertionType->set(m_pEffectChain->insertionType());

        // Mix and enabled channels are persistent properties of the chain slot,
        // not of the chain. Propagate the current settings to the chain.
        m_pEffectChain->setMix(m_pControlChainMix->get());
        m_pEffectChain->setEnabled(m_pControlChainEnabled->get() > 0.0);
        foreach (ChannelInfo* pChannelInfo, m_channelInfoByName) {
            if (pChannelInfo->pEnabled->toBool()) {
                m_pEffectChain->enableForChannel(pChannelInfo->handle_group);
            } else {
                m_pEffectChain->disableForChannel(pChannelInfo->handle_group);
            }
        }

        // Don't emit because we will below.
        onChainEffectsChanged(false);
    }

    emit(effectChainLoaded(pEffectChain));
    emit(updated());
}

EffectChainPointer EffectChainSlot::getEffectChain() const {
    return m_pEffectChain;
}

void EffectChainSlot::clear() {
    // Stop listening to signals from any loaded effect
    if (m_pEffectChain) {
        m_pEffectChain->removeFromEngine(m_pEffectRack->getEngineEffectRack(),
                                         m_iChainSlotNumber);
        foreach (EffectSlotPointer pSlot, m_slots) {
            pSlot->clear();
        }
        m_pEffectChain->disconnect(this);
        m_pEffectChain.clear();
    }
    m_pControlNumEffects->setAndConfirm(0.0);
    m_pControlChainLoaded->setAndConfirm(0.0);
    m_pControlChainInsertionType->set(EffectChain::INSERT);
    emit(updated());
}

unsigned int EffectChainSlot::numSlots() const {
    //qDebug() << debugString() << "numSlots";
    return m_slots.size();
}

EffectSlotPointer EffectChainSlot::addEffectSlot(const QString& group) {
    //qDebug() << debugString() << "addEffectSlot" << group;

    EffectSlot* pEffectSlot = new EffectSlot(group, m_iChainSlotNumber,
                                             m_slots.size());
    // Rebroadcast effectLoaded signals
    connect(pEffectSlot, SIGNAL(effectLoaded(EffectPointer, unsigned int)),
            this, SLOT(onEffectLoaded(EffectPointer, unsigned int)));
    connect(pEffectSlot, SIGNAL(clearEffect(unsigned int)),
            this, SLOT(onClearEffect(unsigned int)));
    connect(pEffectSlot, SIGNAL(nextEffect(unsigned int, unsigned int, EffectPointer)),
            this, SIGNAL(nextEffect(unsigned int, unsigned int, EffectPointer)));
    connect(pEffectSlot, SIGNAL(prevEffect(unsigned int, unsigned int, EffectPointer)),
            this, SIGNAL(prevEffect(unsigned int, unsigned int, EffectPointer)));

    EffectSlotPointer pSlot(pEffectSlot);
    m_slots.append(pSlot);
    m_pControlNumEffectSlots->setAndConfirm(m_pControlNumEffectSlots->get() + 1);
    return pSlot;
}

void EffectChainSlot::registerChannel(const ChannelHandleAndGroup& handle_group) {
    if (m_channelInfoByName.contains(handle_group.name())) {
        qWarning() << debugString()
                   << "WARNING: registerChannel already has channel registered:"
                   << handle_group.name();
        return;
    }
    ControlPushButton* pEnableControl = new ControlPushButton(
            ConfigKey(m_group, QString("group_%1_enable").arg(handle_group.name())));
    pEnableControl->setButtonMode(ControlPushButton::POWERWINDOW);

    ChannelInfo* pInfo = new ChannelInfo(handle_group, pEnableControl);
    m_channelInfoByName[handle_group.name()] = pInfo;
    m_channelStatusMapper.setMapping(pEnableControl, handle_group.name());
    connect(pEnableControl, SIGNAL(valueChanged(double)),
            &m_channelStatusMapper, SLOT(map()));
}

void EffectChainSlot::onEffectLoaded(EffectPointer pEffect, unsigned int onNumber) {
    // const int is a safe read... don't bother locking
    emit(effectLoaded(pEffect, m_iChainSlotNumber, onNumber));
}

void EffectChainSlot::onClearEffect(unsigned int iEffectSlotNumber) {
    if (m_pEffectChain) {
        m_pEffectChain->removeEffect(iEffectSlotNumber);
    }
}

EffectSlotPointer EffectChainSlot::getEffectSlot(unsigned int onNumber) {
    //qDebug() << debugString() << "getEffectSlot" << onNumber;
    if (onNumber >= static_cast<unsigned int>(m_slots.size())) {
        qWarning() << "WARNING: onNumber out of range";
        return EffectSlotPointer();
    }
    return m_slots[onNumber];
}

void EffectChainSlot::onControlClear(double v) {
    if (v > 0) {
        clear();
    }
}

void EffectChainSlot::onControlNumEffects(double v) {
    // Ignore sets to num_effects.
    Q_UNUSED(v);
    //qDebug() << debugString() << "onControlNumEffects" << v;
    qWarning() << "WARNING: num_effects is a read-only control.";
}

void EffectChainSlot::onControlNumEffectSlots(double v) {
    // Ignore sets to num_effectslots.
    Q_UNUSED(v);
    //qDebug() << debugString() << "onControlNumEffectSlots" << v;
    qWarning() << "WARNING: num_effectslots is a read-only control.";
}

void EffectChainSlot::onControlChainLoaded(double v) {
    // Ignore sets to loaded.
    Q_UNUSED(v);
    //qDebug() << debugString() << "onControlChainLoaded" << v;
    qWarning() << "WARNING: loaded is a read-only control.";
}

void EffectChainSlot::onControlChainEnabled(double v) {
    //qDebug() << debugString() << "onControlChainEnabled" << v;
    if (m_pEffectChain) {
        m_pEffectChain->setEnabled(v > 0);
    }
}

void EffectChainSlot::onControlChainMix(double v) {
    //qDebug() << debugString() << "onControlChainMix" << v;

    // Clamp to [0.0, 1.0]
    if (v < 0.0 || v > 1.0) {
        qWarning() << debugString() << "value out of limits";
        v = math_clamp(v, 0.0, 1.0);
        m_pControlChainMix->set(v);
    }
    if (m_pEffectChain) {
        m_pEffectChain->setMix(v);
    }
}

void EffectChainSlot::onControlChainSuperParameter(double v) {
    //qDebug() << debugString() << "onControlChainSuperParameter" << v;

    // Clamp to [0.0, 1.0]
    if (v < 0.0 || v > 1.0) {
        qWarning() << debugString() << "value out of limits";
        v = math_clamp(v, 0.0, 1.0);
        m_pControlChainSuperParameter->set(v);
    }
    for (int i = 0; i < m_slots.size(); ++i) {
        m_slots[i]->onChainSuperParameterChanged(v);
    }
}

void EffectChainSlot::onControlChainInsertionType(double v) {
    // Intermediate cast to integer is needed for VC++.
    EffectChain::InsertionType type = static_cast<EffectChain::InsertionType>(int(v));
    (void)v; // this avoids a false warning with g++ 4.8.1
    if (m_pEffectChain && type >= 0 &&
            type < EffectChain::NUM_INSERTION_TYPES) {
        m_pEffectChain->setInsertionType(type);
    }
}

void EffectChainSlot::onControlChainSelector(double v) {
    //qDebug() << debugString() << "onControlChainSelector" << v;
    if (v > 0) {
        emit(nextChain(m_iChainSlotNumber, m_pEffectChain));
    } else if (v < 0) {
        emit(prevChain(m_iChainSlotNumber, m_pEffectChain));
    }
}

void EffectChainSlot::onControlChainNextPreset(double v) {
    //qDebug() << debugString() << "onControlChainNextPreset" << v;
    if (v > 0) {
        onControlChainSelector(1);
    }
}

void EffectChainSlot::onControlChainPrevPreset(double v) {
    //qDebug() << debugString() << "onControlChainPrevPreset" << v;
    if (v > 0) {
        onControlChainSelector(-1);
    }
}

void EffectChainSlot::onChannelStatusChanged(const QString& group) {
    if (m_pEffectChain) {
        ChannelInfo* pChannelInfo = m_channelInfoByName.value(group, NULL);
        if (pChannelInfo != NULL && pChannelInfo->pEnabled != NULL) {
            bool bEnable = pChannelInfo->pEnabled->toBool();
            if (bEnable) {
                m_pEffectChain->enableForChannel(pChannelInfo->handle_group);
            } else {
                m_pEffectChain->disableForChannel(pChannelInfo->handle_group);
            }
        }
    }
}

unsigned int EffectChainSlot::getChainSlotNumber() const {
    return m_iChainSlotNumber;
}
