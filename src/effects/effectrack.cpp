#include "effects/effectrack.h"

#include "effects/effectsmanager.h"
#include "effects/effectchainmanager.h"
#include "engine/effects/engineeffectrack.h"
#include "control/control.h"
#include "controlobject.h"
EffectRack::EffectRack(EffectsManager* pEffectsManager,
                       EffectChainManager* pEffectChainManager,
                       const unsigned int iRackNumber,
                       QString group)
        : m_pEffectsManager(pEffectsManager),
          m_pEffectChainManager(pEffectChainManager),
          m_iRackNumber(iRackNumber),
          m_group(group),
          m_controlNumEffectChainSlots(new ControlObject(ConfigKey(m_group, "num_effectunits"))),
          m_controlClearRack(new ControlObject(ConfigKey(m_group, "clear"))),
          m_pEngineEffectRack(nullptr) {
    connect(m_controlClearRack, SIGNAL(valueChanged(double)),this, SLOT(slotClearRack(double)));
    m_controlNumEffectChainSlots->connectValueChangeRequest(this, SLOT(slotNumEffectChainSlots(double)));
    addToEngine();
}
EffectRack::~EffectRack()
{
    removeFromEngine();
    //qDebug() << "EffectRack::~EffectRack()";
}
EngineEffectRack* EffectRack::getEngineEffectRack()
{
    return m_pEngineEffectRack;
}
void EffectRack::addToEngine()
{
    m_pEngineEffectRack = new EngineEffectRack(m_iRackNumber);
    auto  pRequest = new EffectsRequest();
    pRequest->type = EffectsRequest::ADD_EFFECT_RACK;
    pRequest->AddEffectRack.pRack = m_pEngineEffectRack;
    m_pEffectsManager->writeRequest(pRequest);

    // Add all effect chains.
    for (auto i = 0; i < m_effectChainSlots.size(); ++i) {
        auto pSlot = m_effectChainSlots[i];
        auto pChain = pSlot->getEffectChain();
        if (pChain)
        {
            // Add the effect to the engine.
            pChain->addToEngine(m_pEngineEffectRack, i);
            // Update its parameters in the engine.
            pChain->updateEngineState();
        }
    }
}

void EffectRack::removeFromEngine()
{
    // Order doesn't matter when removing.
    for (auto i = 0; i < m_effectChainSlots.size(); ++i) {
        auto pSlot = m_effectChainSlots[i];
        auto pChain = pSlot->getEffectChain();
        if (pChain) {pChain->removeFromEngine(m_pEngineEffectRack, i);}
    }

    auto  pRequest = new EffectsRequest();
    pRequest->type = EffectsRequest::REMOVE_EFFECT_RACK;
    pRequest->RemoveEffectRack.pRack = m_pEngineEffectRack;
    m_pEffectsManager->writeRequest(pRequest);
    m_pEngineEffectRack = nullptr;
}

void EffectRack::registerChannel(const ChannelHandleAndGroup& handle_group) {
    for(auto pChainSlot: m_effectChainSlots)pChainSlot->registerChannel(handle_group);
}

void EffectRack::slotNumEffectChainSlots(double v) {
    // Ignore sets to num_effectchain_slots
    Q_UNUSED(v);
    //qDebug() << debugString() << "slotNumEffectChainSlots" << v;
    qWarning() << "WARNING: num_effectchain_slots is a read-only control.";
}

void EffectRack::slotClearRack(double v) {
    if (v > 0) {
        for(auto pChainSlot: m_effectChainSlots) pChainSlot->clear();
    }
}

EffectChainPointer EffectRack::makeEmptyChain()
{
    auto pChain = EffectChainPointer(new EffectChain(m_pEffectsManager, QString(),EffectChainPointer()));
    pChain->setName(tr("Empty Chain"));
    return pChain;
}

int EffectRack::numEffectChainSlots() const
{
    return m_effectChainSlots.size();
}
void EffectRack::addEffectChainSlotInternal(EffectChainSlotPointer pChainSlot) {
    m_effectChainSlots.append(pChainSlot);
    m_controlNumEffectChainSlots->setAndConfirm(
        m_controlNumEffectChainSlots->get() + 1);
}

EffectChainSlotPointer EffectRack::getEffectChainSlot(int i) {
    if (i < 0 || i >= m_effectChainSlots.size()) {
        qWarning() << "WARNING: Invalid index for getEffectChainSlot";
        return EffectChainSlotPointer();
    }
    return m_effectChainSlots[i];
}

void EffectRack::loadNextChain(const unsigned int iChainSlotNumber,
                               EffectChainPointer pLoadedChain) {
    if (pLoadedChain) pLoadedChain = pLoadedChain->prototype();

    auto pNextChain = m_pEffectChainManager->getNextEffectChain(pLoadedChain);

    pNextChain = EffectChain::clone(pNextChain);
    m_effectChainSlots[iChainSlotNumber]->loadEffectChain(pNextChain);
}


void EffectRack::loadPrevChain(const unsigned int iChainSlotNumber,
                               EffectChainPointer pLoadedChain) {
    if (pLoadedChain) {
        pLoadedChain = pLoadedChain->prototype();
    }

    auto pPrevChain = m_pEffectChainManager->getPrevEffectChain(pLoadedChain);

    pPrevChain = EffectChain::clone(pPrevChain);
    m_effectChainSlots[iChainSlotNumber]->loadEffectChain(pPrevChain);
}

void EffectRack::loadNextEffect(const unsigned int iChainSlotNumber,
                                const unsigned int iEffectSlotNumber,
                                EffectPointer pEffect) {
    if (iChainSlotNumber >= static_cast<unsigned int>(m_effectChainSlots.size()))return;
    auto effectId = pEffect ? pEffect->getManifest().id() : QString();
    auto nextEffectId = m_pEffectsManager->getNextEffectId(effectId);
    auto pNextEffect = m_pEffectsManager->instantiateEffect(nextEffectId);

    auto pChainSlot = m_effectChainSlots[iChainSlotNumber];
    auto pChain = pChainSlot->getEffectChain();
    if (!pChain) {
        pChain = makeEmptyChain();
        pChainSlot->loadEffectChain(pChain);
    }
    pChain->replaceEffect(iEffectSlotNumber, pNextEffect);
}


void EffectRack::loadPrevEffect(const unsigned int iChainSlotNumber,
                                const unsigned int iEffectSlotNumber,
                                EffectPointer pEffect) {
    if (iChainSlotNumber >= static_cast<unsigned int>(m_effectChainSlots.size())) {
        return;
    }

    auto effectId = pEffect ? pEffect->getManifest().id() : QString();
    auto prevEffectId = m_pEffectsManager->getPrevEffectId(effectId);
    auto pPrevEffect = m_pEffectsManager->instantiateEffect(prevEffectId);

    auto pChainSlot = m_effectChainSlots[iChainSlotNumber];
    auto pChain = pChainSlot->getEffectChain();
    if (!pChain)
    {
        pChain = makeEmptyChain();
        pChainSlot->loadEffectChain(pChain);
    }

    pChain->replaceEffect(iEffectSlotNumber, pPrevEffect);
}

StandardEffectRack::StandardEffectRack(EffectsManager* pEffectsManager,
                                       EffectChainManager* pChainManager,
                                       const unsigned int iRackNumber)
        : EffectRack(pEffectsManager, pChainManager, iRackNumber,
                     formatGroupString(iRackNumber)) {
}

EffectChainSlotPointer StandardEffectRack::addEffectChainSlot() {
    auto iChainSlotNumber = numEffectChainSlots();

    auto group = formatEffectChainSlotGroupString(getRackNumber(),
                                                     iChainSlotNumber);
    auto pChainSlot = new EffectChainSlot(this, group, iChainSlotNumber);

    // TODO(rryan) How many should we make default? They create controls that
    // the GUI may rely on, so the choice is important to communicate to skin
    // designers.
    for (auto i = 0; i < 4; ++i) {
        pChainSlot->addEffectSlot(
            StandardEffectRack::formatEffectSlotGroupString(getRackNumber(), iChainSlotNumber, i));
    }
    connect(pChainSlot, SIGNAL(nextChain(unsigned int, EffectChainPointer)),
            this, SLOT(loadNextChain(unsigned int, EffectChainPointer)));
    connect(pChainSlot, SIGNAL(prevChain(unsigned int, EffectChainPointer)),
            this, SLOT(loadPrevChain(unsigned int, EffectChainPointer)));

    connect(pChainSlot, SIGNAL(nextEffect(unsigned int, unsigned int, EffectPointer)),
            this, SLOT(loadNextEffect(unsigned int, unsigned int, EffectPointer)));
    connect(pChainSlot, SIGNAL(prevEffect(unsigned int, unsigned int, EffectPointer)),
            this, SLOT(loadPrevEffect(unsigned int, unsigned int, EffectPointer)));

    // Register all the existing channels with the new EffectChain.
    auto registeredChannels = m_pEffectChainManager->registeredChannels();
    for(auto & handle_group: registeredChannels) pChainSlot->registerChannel(handle_group);

    auto pChainSlotPointer = EffectChainSlotPointer(pChainSlot);
    addEffectChainSlotInternal(pChainSlotPointer);

    // Now load an empty effect chain into the slot so that users can edit
    // effect slots on the fly without having to load a chain.
    auto pChain = makeEmptyChain();
    pChainSlotPointer->loadEffectChain(pChain);

    return pChainSlotPointer;
}

PerGroupRack::PerGroupRack(EffectsManager* pEffectsManager,
                           EffectChainManager* pChainManager,
                           const unsigned int iRackNumber,
                           QString group)
        : EffectRack(pEffectsManager, pChainManager, iRackNumber, group) {
}

EffectChainSlotPointer PerGroupRack::addEffectChainSlotForGroup(QString groupName) {
    if (m_groupToChainSlot.contains(groupName)) {
        qWarning() << "PerGroupRack" << getGroup()
                   << "group is already registered" << groupName;
        return getGroupEffectChainSlot(groupName);
    }

    auto iChainSlotNumber = m_groupToChainSlot.size();
    auto chainSlotGroup = formatEffectChainSlotGroupForGroup(
        getRackNumber(), iChainSlotNumber, groupName);
    auto pChainSlot = new EffectChainSlot(this, chainSlotGroup,
                                                      iChainSlotNumber);
    auto pChainSlotPointer = EffectChainSlotPointer(pChainSlot);
    addEffectChainSlotInternal(pChainSlotPointer);
    m_groupToChainSlot[groupName] = pChainSlotPointer;

    // TODO(rryan): remove.
    for(auto & handle_group:m_pEffectChainManager->registeredChannels())
    {
        if (handle_group.name() == groupName) {
            configureEffectChainSlotForGroup(pChainSlotPointer, handle_group);
            return pChainSlotPointer;
        }
    }

    qWarning() << "PerGroupRack::addEffectChainSlotForGroup" << groupName
               << "was not registered before calling."
               << "Can't configure the effect chain slot.";
    return pChainSlotPointer;
}

EffectChainSlotPointer PerGroupRack::getGroupEffectChainSlot(QString group)
{
    return m_groupToChainSlot[group];
}

QuickEffectRack::QuickEffectRack(EffectsManager* pEffectsManager,
                                 EffectChainManager* pChainManager,
                                 const unsigned int iRackNumber)
        : PerGroupRack(pEffectsManager, pChainManager, iRackNumber,QuickEffectRack::formatGroupString(iRackNumber)) 
{
}

void QuickEffectRack::configureEffectChainSlotForGroup(EffectChainSlotPointer pSlot, const ChannelHandleAndGroup& handle_group)
{
    // Register this channel alone with the chain slot.
    pSlot->registerChannel(handle_group);
    // Add a single EffectSlot for the quick effect.
    pSlot->addEffectSlot(QuickEffectRack::formatEffectSlotGroupString(getRackNumber(), handle_group.name()));
    // TODO(rryan): Set up next/prev signals.
    // Now load an empty effect chain into the slot so that users can edit
    // effect slots on the fly without having to load a chain.
    auto pChain = makeEmptyChain();
    pSlot->loadEffectChain(pChain);
    // Enable the chain for the channel by default.
    pChain->enableForChannel(handle_group);
    // Set the chain to be fully wet.
    pChain->setMix(1.0);
    // Set the parameter default value to 0.5 (neutral).
    pSlot->setSuperParameter(0.5);
    pSlot->setSuperParameterDefaultValue(0.5);
}

bool QuickEffectRack::loadEffectToGroup(QString groupName,EffectPointer pEffect) {
    auto pChainSlot = getGroupEffectChainSlot(groupName);
    if (pChainSlot.isNull()) {
        qWarning() << "No chain for group" << groupName;
        return false;
    }
    auto pChain = pChainSlot->getEffectChain();
    if (pChain.isNull()) {
        pChain = makeEmptyChain();
        pChainSlot->loadEffectChain(pChain);
        // TODO(rryan): remove.
        for(auto & handle_group:m_pEffectChainManager->registeredChannels()) {
            if (handle_group.name() == groupName) pChain->enableForChannel(handle_group);
        }
        pChain->setMix(1.0);
    }
    pChain->replaceEffect(0, pEffect);
    // Force update the new effect to match the current superknob position.
    auto  pEffectSlot = pChainSlot->getEffectSlot(0);
    if (pEffectSlot) {
        pEffectSlot->onChainSuperParameterChanged(
                pChainSlot->getSuperParameter(), true);
    }
    return true;
}

EqualizerRack::EqualizerRack(EffectsManager* pEffectsManager,
                             EffectChainManager* pChainManager,
                             const unsigned int iRackNumber)
        : PerGroupRack(pEffectsManager, pChainManager, iRackNumber,
                       EqualizerRack::formatGroupString(iRackNumber)) {
}

bool EqualizerRack::loadEffectToGroup(QString groupName,EffectPointer pEffect)
{
    auto pChainSlot = getGroupEffectChainSlot(groupName);
    if (pChainSlot.isNull()) {
        qWarning() << "No chain for group" << groupName;
        return false;
    }

    auto pChain = pChainSlot->getEffectChain();
    if (pChain.isNull()) {
        pChain = makeEmptyChain();
        pChainSlot->loadEffectChain(pChain);
        // TODO(rryan): remove.
        for(auto& handle_group: m_pEffectChainManager->registeredChannels())
        {
            if (handle_group.name() == groupName) pChain->enableForChannel(handle_group);
        }
        pChain->setMix(1.0);
    }

    pChain->replaceEffect(0, pEffect);
    return true;
}


void EqualizerRack::configureEffectChainSlotForGroup(EffectChainSlotPointer pSlot,
                                                     const ChannelHandleAndGroup& handle_group) {
    auto groupName = handle_group.name();

    // Register this channel alone with the chain slot.
    pSlot->registerChannel(handle_group);

    // Add a single EffectSlot for the equalizer effect.
    pSlot->addEffectSlot(EqualizerRack::formatEffectSlotGroupString(
            getRackNumber(), groupName));

    // TODO(rryan): Set up next/prev signals.

    // Now load an empty effect chain into the slot so that users can edit
    // effect slots on the fly without having to load a chain.
    auto pChain = makeEmptyChain();
    pSlot->loadEffectChain(pChain);

    // Enable the chain for the channel by default.
    pChain->enableForChannel(handle_group);

    // Set the chain to be fully wet.
    pChain->setMix(1.0);

    // Create aliases for legacy EQ controls.
    // NOTE(rryan): If we ever add a second EqualizerRack then we need to make
    // these only apply to the first.
    auto pEffectSlot = pSlot->getEffectSlot(0);
    if (pEffectSlot) {
        auto effectSlotGroup = pEffectSlot->getGroup();
        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterLow"),
                                          ConfigKey(effectSlotGroup, "parameter1"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterMid"),
                                          ConfigKey(effectSlotGroup, "parameter2"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterHigh"),
                                          ConfigKey(effectSlotGroup, "parameter3"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterLowKill"),
                                          ConfigKey(effectSlotGroup, "button_parameter1"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterMidKill"),
                                          ConfigKey(effectSlotGroup, "button_parameter2"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterHighKill"),
                                          ConfigKey(effectSlotGroup, "button_parameter3"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterLow_loaded"),
                                          ConfigKey(effectSlotGroup, "parameter1_loaded"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterMid_loaded"),
                                          ConfigKey(effectSlotGroup, "parameter2_loaded"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterHigh_loaded"),
                                          ConfigKey(effectSlotGroup, "parameter3_loaded"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterLowKill_loaded"),
                                          ConfigKey(effectSlotGroup, "button_parameter1_loaded"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterMidKill_loaded"),
                                          ConfigKey(effectSlotGroup, "button_parameter2_loaded"));

        ControlDoublePrivate::insertAlias(ConfigKey(groupName, "filterHighKill_loaded"),
                                          ConfigKey(effectSlotGroup, "button_parameter3_loaded"));
    }
}
