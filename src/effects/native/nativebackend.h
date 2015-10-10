_Pragma("once")
#include "effects/effectsbackend.h"
class NativeBackend : public EffectsBackend {
    Q_OBJECT
  public:
    NativeBackend(QObject* pParent=nullptr);
    virtual ~NativeBackend();
  private:
    QString debugString() const;
};
