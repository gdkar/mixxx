_Pragma("once")
#include <QDomNode>

#include "widget/wlabel.h"
#include "widget/weffectparameterbase.h"
#include "skin/skincontext.h"

class EffectsManager;

class WEffectParameter : public WEffectParameterBase {
    Q_OBJECT
  public:
    WEffectParameter(QWidget* pParent, EffectsManager* pEffectsManager);
    virtual ~WEffectParameter();
    void setup(QDomNode node, const SkinContext& context);
};
