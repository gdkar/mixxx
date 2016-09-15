#include <QtDebug>

#include "widget/weffectbuttonparameter.h"
#include "effects/effectsmanager.h"
#include "widget/effectwidgetutils.h"

WEffectButtonParameter::WEffectButtonParameter(QWidget* pParent, EffectsManager* pEffectsManager)
        : WEffectParameterBase(pParent, pEffectsManager)
        {
}

void WEffectButtonParameter::setup(const QDomNode& node, const SkinContext& context) {
    // EffectWidgetUtils propagates NULLs so this is all safe.
    auto pRack = EffectWidgetUtils::getEffectRackFromNode(
        node, context, m_pEffectsManager);
    auto pChainSlot = EffectWidgetUtils::getEffectChainSlotFromNode(
        node, context, pRack);
    auto pEffectSlot = EffectWidgetUtils::getEffectSlotFromNode(
        node, context, pChainSlot);
    auto pParameterSlot =EffectWidgetUtils::getButtonParameterSlotFromNode(
        node, context, pEffectSlot);

    if (pParameterSlot) {
        setEffectParameterSlot(pParameterSlot);
    } else {
        SKIN_WARNING(node, context)
                << "EffectButtonParameter node could not attach to effect parameter";
    }
}
