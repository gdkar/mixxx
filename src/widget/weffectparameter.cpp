#include <QtDebug>

#include "widget/weffectparameter.h"
#include "effects/effectsmanager.h"
#include "widget/effectwidgetutils.h"

WEffectParameter::WEffectParameter(QWidget* pParent, EffectsManager* pEffectsManager)
        : WEffectParameterBase(pParent, pEffectsManager) {
}

void WEffectParameter::setup(const QDomNode& node, const SkinContext& context) {
    // EffectWidgetUtils propagates NULLs so this is all safe.
    auto pRack = EffectWidgetUtils::getEffectRackFromNode(node, context, m_pEffectsManager);
    auto pChainSlot = EffectWidgetUtils::getEffectChainSlotFromNode(node, context, pRack);
    auto pEffectSlot = EffectWidgetUtils::getEffectSlotFromNode(node, context, pChainSlot);
    auto pParameterSlot =EffectWidgetUtils::getParameterSlotFromNode(node, context, pEffectSlot);
    if (pParameterSlot) {
        setEffectParameterSlot(pParameterSlot);
    } else {
        SKIN_WARNING(node, context)
                << "EffectParameter node could not attach to effect parameter";
    }
}
