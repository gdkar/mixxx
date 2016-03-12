_Pragma("once")
#include "effects/effectsbackend.h"

class NativeBackend : public EffectsBackend {
    Q_OBJECT
  public:
    NativeBackend(QObject* pParent=NULL);
    virtual ~NativeBackend();

  private:
    QString debugString() const {
        return "NativeBackend";
    }
};
