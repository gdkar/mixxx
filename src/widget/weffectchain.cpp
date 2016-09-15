#include <QtDebug>

#include "widget/weffectchain.h"
#include "effects/effectsmanager.h"
#include "widget/effectwidgetutils.h"

WEffectChain::WEffectChain(QWidget* pParent, EffectsManager* pEffectsManager)
        : WLabel(pParent),
          m_pEffectsManager(pEffectsManager)
{
    chainUpdated();
}

void WEffectChain::setup(const QDomNode& node, const SkinContext& context)
{
    WLabel::setup(node, context);
    // EffectWidgetUtils propagates NULLs so this is all safe.
    auto pRack = EffectWidgetUtils::getEffectRackFromNode(node, context, m_pEffectsManager);
    auto pChainSlot = EffectWidgetUtils::getEffectChainSlotFromNode(node, context, pRack);
    if (pChainSlot) {
        setEffectChainSlot(pChainSlot);
    } else {
        SKIN_WARNING(node, context)
                << "EffectChain node could not attach to effect chain slot.";
    }
}

void WEffectChain::setEffectChainSlot(EffectChainSlotPointer pEffectChainSlot)
{
    if (pEffectChainSlot) {
        m_pEffectChainSlot = pEffectChainSlot;
        connect(pEffectChainSlot.data(), SIGNAL(updated()),this, SLOT(chainUpdated()));
        chainUpdated();
    }
}

void WEffectChain::chainUpdated()
{
    auto name = tr("None");
    auto description = tr("No effect chain loaded.");
    if (m_pEffectChainSlot) {
        if(auto pChain = m_pEffectChainSlot->getEffectChain()){
            name = pChain->name();
            description = pChain->description();
        }
    }
    setText(name);
    setBaseTooltip(description);
}
