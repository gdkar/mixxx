#include <QtDebug>

#include "widget/weffect.h"

#include "effects/effectsmanager.h"
#include "widget/effectwidgetutils.h"

WEffect::WEffect(QWidget* pParent, EffectsManager* pEffectsManager)
        : WLabel(pParent),
          m_pEffectsManager(pEffectsManager)
{
    effectUpdated();
}
WEffect::~WEffect()  = default;

void WEffect::setup(QDomNode node, const SkinContext& context) {
    WLabel::setup(node, context);
    // EffectWidgetUtils propagates NULLs so this is all safe.
    auto pRack = EffectWidgetUtils::getEffectRackFromNode(node, context, m_pEffectsManager);
    auto pChainSlot = EffectWidgetUtils::getEffectChainSlotFromNode(node, context, pRack);
    auto pEffectSlot = EffectWidgetUtils::getEffectSlotFromNode( node, context, pChainSlot);
    if (pEffectSlot) setEffectSlot(pEffectSlot);
    else SKIN_WARNING(node, context)<< "EffectName node could not attach to effect slot.";
}

void WEffect::setEffectSlot(EffectSlotPointer pEffectSlot)
{
    if (pEffectSlot)
    {
        m_pEffectSlot = pEffectSlot;
        connect(pEffectSlot.data(), SIGNAL(updated()),this, SLOT(effectUpdated()));
        effectUpdated();
    }
}
void WEffect::effectUpdated()
{
    if (m_pEffectSlot)
    {
        auto pEffect = m_pEffectSlot->getEffect();
        if (pEffect)
        {
            auto manifest = pEffect->getManifest();
            setText(manifest.name());
            setBaseTooltip(manifest.description());
        }
    }
    else
    {
        setText(tr("None"));
        setBaseTooltip(tr("No effect loaded."));
    }
}
