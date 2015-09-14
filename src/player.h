_Pragma("once")
#include <QObject>
#include "configobject.h"
#include "trackinfoobject.h"
#include "engine/enginechannel.h"
#include "engine/enginedeck.h"

class EngineMaster;
class ControlObject;
class ControlObjectSlave;
class AnalyserQueue;
class EffectsManager;
class Player : public QObject {
    Q_OBJECT
  public:
    enum TrackLoadReset {
        RESET_NONE,
        RESET_PITCH,
        RESET_PITCH_AND_SPEED,
    };
    Q_ENUM(TrackLoadReset);
    Player(QObject* pParent,ConfigObject<ConfigValue>* pConfig,
                        EngineMaster* pMixingEngine,
                        EffectsManager* pEffectsManager,
                        EngineChannel::ChannelOrientation defaultOrientation,
                        QString group,
                        bool defaultMaster,

                        bool defaultHeadphones);
    virtual ~Player();
    TrackPointer getLoadedTrack() const;
    // TODO(XXX): Only exposed to let the passthrough AudioInput get
    // connected. Delete me when EngineMaster supports AudioInput assigning.
    EngineDeck* getEngineDeck() const;
    void setupEqControls();
    QString getGroup()const;
  public slots:
    virtual void slotLoadTrack(TrackPointer track, bool bPlay=false);
    virtual void slotFinishLoading(TrackPointer pTrackInfoObject);
    virtual void slotLoadFailed(TrackPointer pTrackInfoObject, QString reason);
    virtual void slotUnloadTrack(TrackPointer track);
    virtual void slotSetReplayGain(double replayGain);
    virtual void slotPlayToggled(double);
  signals:
    void loadTrack(TrackPointer, bool bPlay=false);
    void loadTrackFailed(TrackPointer);
    void newTrackLoaded(TrackPointer);
    void unloadingTrack(TrackPointer);
  private:
    const QString m_group;
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    TrackPointer m_pLoadedTrack;
    // Waveform display related controls
    ControlObject* m_pWaveformZoom = nullptr;
    ControlObject* m_pEndOfTrack = nullptr;
    ControlObjectSlave* m_pLoopInPoint = nullptr;
    ControlObjectSlave* m_pLoopOutPoint = nullptr;
    ControlObject* m_pDuration = nullptr;
    ControlObjectSlave* m_pBPM = nullptr;
    ControlObjectSlave* m_pKey = nullptr;
    ControlObjectSlave* m_pReplayGain = nullptr;
    ControlObjectSlave* m_pPlay = nullptr;
    ControlObjectSlave* m_pLowFilter = nullptr;
    ControlObjectSlave* m_pMidFilter = nullptr;
    ControlObjectSlave* m_pHighFilter = nullptr;
    ControlObjectSlave* m_pLowFilterKill = nullptr;
    ControlObjectSlave* m_pMidFilterKill = nullptr;
    ControlObjectSlave* m_pHighFilterKill = nullptr;
    ControlObjectSlave* m_pPreGain = nullptr;
    ControlObjectSlave* m_pSpeed = nullptr;
    ControlObjectSlave* m_pPitchAdjust = nullptr;
    EngineDeck* m_pChannel = nullptr;
    bool m_replaygainPending = false;
    
};
