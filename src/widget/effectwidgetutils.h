_Pragma("once")
#include <QDomNode>

#include "effects/effectsmanager.h"
#include "skin/skincontext.h"

class EffectWidgetUtils {
  public:
    static EffectRackPointer getEffectRackFromNode(const QDomNode& node,const SkinContext& context,EffectsManager* pEffectsManager);
    static EffectChainSlotPointer getEffectChainSlotFromNode(const QDomNode& node,const SkinContext& context,EffectRackPointer pRack);
    static EffectSlotPointer getEffectSlotFromNode(const QDomNode& node,const SkinContext& context,EffectChainSlotPointer pChainSlot);
    static EffectParameterSlotBasePointer getParameterSlotFromNode(const QDomNode& node,const SkinContext& context,EffectSlotPointer pEffectSlot);
    static EffectParameterSlotBasePointer getButtonParameterSlotFromNode(const QDomNode& node,const SkinContext& context,EffectSlotPointer pEffectSlot);
  private:
    EffectWidgetUtils() = delete;
};
