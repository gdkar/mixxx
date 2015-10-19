_Pragma("once")
#include <QObject>
#include "configobject.h"
#include "trackinfoobject.h"
#include "engine/enginechannel.h"
#include "engine/enginedeck.h"

class EngineMaster;
class ControlObject;
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
    ControlObject* m_pLoopInPoint = nullptr;
    ControlObject* m_pLoopOutPoint = nullptr;
    ControlObject* m_pDuration = nullptr;
    ControlObject* m_pBPM = nullptr;
    ControlObject* m_pKey = nullptr;
    ControlObject* m_pReplayGain = nullptr;
    ControlObject* m_pPlay = nullptr;
    ControlObject* m_pLowFilter = nullptr;
    ControlObject* m_pMidFilter = nullptr;
    ControlObject* m_pHighFilter = nullptr;
    ControlObject* m_pLowFilterKill = nullptr;
    ControlObject* m_pMidFilterKill = nullptr;
    ControlObject* m_pHighFilterKill = nullptr;
    ControlObject* m_pPreGain = nullptr;
    ControlObject* m_pSpeed = nullptr;
    ControlObject* m_pPitchAdjust = nullptr;
    EngineDeck* m_pChannel = nullptr;
    bool m_replaygainPending = false;
    
};
