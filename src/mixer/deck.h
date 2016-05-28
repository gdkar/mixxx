_Pragma("once")
#include <QObject>

#include "mixer/basetrackplayer.h"

class Deck : public BaseTrackPlayerImpl {
    Q_OBJECT
  public:
    Deck(QObject* pParent,
         UserSettingsPointer pConfig,
         EngineMaster* pMixingEngine,
         EffectsManager* pEffectsManager,
         EngineChannel::ChannelOrientation defaultOrientation,
         const QString& group);
    virtual ~Deck();
};
