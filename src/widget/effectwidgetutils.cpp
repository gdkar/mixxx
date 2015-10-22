#include "widget/effectwidgetutils.h"

EffectRackPointer EffectWidgetUtils::getEffectRackFromNode(const QDomNode& node,const SkinContext& context,EffectsManager* pEffectsManager)
{
    if (!pEffectsManager) return EffectRackPointer();
    // If specified, EffectRack always refers to a StandardEffectRack index.
    auto rackNumberOk = false;
    auto rackNumber   = context.selectInt(node, "EffectRack",&rackNumberOk);
    if (rackNumberOk) return pEffectsManager->getStandardEffectRack(rackNumber - 1);
    // For custom racks, users can specify EffectRackGroup explicitly
    // instead.
    auto  rackGroup = QString{};
    if (!context.hasNodeSelectString(node, "EffectRackGroup", &rackGroup)) return EffectRackPointer();
    return pEffectsManager->getEffectRack(rackGroup);
}
EffectChainSlotPointer EffectWidgetUtils::getEffectChainSlotFromNode(const QDomNode& node,const SkinContext& context,EffectRackPointer pRack)
{
    if (pRack.isNull()) return EffectChainSlotPointer();
    auto unitNumberOk = false;
    auto unitNumber   = context.selectInt(node, "EffectUnit", &unitNumberOk);
    if (unitNumberOk) return pRack->getEffectChainSlot(unitNumber - 1);
    auto unitGroup = QString{};
    if (!context.hasNodeSelectString(node, "EffectUnitGroup", &unitGroup)) return EffectChainSlotPointer();
    for (auto i = 0; i < pRack->numEffectChainSlots(); ++i)
    {
        auto pSlot = pRack->getEffectChainSlot(i);
        if (pSlot->getGroup() == unitGroup) return pSlot;
    }
    return EffectChainSlotPointer();
}
EffectSlotPointer EffectWidgetUtils::getEffectSlotFromNode(const QDomNode& node,const SkinContext& context,EffectChainSlotPointer pChainSlot)
{
    if (pChainSlot.isNull()) return EffectSlotPointer();
    auto effectSlotOk = false;
    auto effectSlot = context.selectInt(node, "Effect", &effectSlotOk);
    if (effectSlotOk) return pChainSlot->getEffectSlot(effectSlot - 1);
    return EffectSlotPointer();
}
EffectParameterSlotBasePointer EffectWidgetUtils::getParameterSlotFromNode(const QDomNode& node,const SkinContext& context,EffectSlotPointer pEffectSlot)
{
    if (pEffectSlot.isNull()) return EffectParameterSlotBasePointer();
    auto parameterNumberOk = false;
    auto parameterNumber = context.selectInt(node, "EffectParameter", &parameterNumberOk);
    if (parameterNumberOk) return pEffectSlot->getEffectParameterSlot(parameterNumber - 1);
    return EffectParameterSlotBasePointer();
}
EffectParameterSlotBasePointer EffectWidgetUtils::getButtonParameterSlotFromNode(const QDomNode& node,const SkinContext& context,EffectSlotPointer pEffectSlot)
{
    if (pEffectSlot.isNull()) return EffectParameterSlotBasePointer();
    auto parameterNumberOk = false;
    auto parameterNumber = context.selectInt(node, "EffectButtonParameter", &parameterNumberOk);
    if (parameterNumberOk)  return pEffectSlot->getEffectButtonParameterSlot(parameterNumber - 1);
    return EffectParameterSlotBasePointer();
}
