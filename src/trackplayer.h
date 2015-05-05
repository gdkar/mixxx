#ifndef BASETRACKPLAYER_H
#define BASETRACKPLAYER_H
#include <qhash.h>
#include <qobject.h>
#include <qsharedpointer.h>
#include <qatomic.h>

#include "configobject.h"
#include "track/trackinfoobject.h"
#include "engine/enginechannel.h"
#include "engine/enginedeck.h"

class EngineMaster;
class ControlObject;
class ControlPotmeter;
class ControlObjectThread;
class ControlObjectSlave;
class AnalyserQueue;
class EffectsManager;

class TrackPlayer : public QObject {
    Q_OBJECT
    Q_ENUMS(TrackLoadReset);
    Q_PROPERTY(QString group READ getGroup CONSTANT );

  public:
    enum TrackLoadReset {
        RESET_NONE            = 0,
        RESET_PITCH           = 1,
        RESET_SPEED           = 2,
        RESET_PITCH_AND_SPEED = 3,
    };
    Q_DECLARE_FLAGS(TrackLoadFlags,TrackLoadReset);
    TrackPlayer(QObject* pParent,
                        ConfigObject<ConfigValue>* pConfig,
                        EngineMaster* pMixingEngine,
                        EffectsManager* pEffectsManager,
                        EngineChannel::ChannelOrientation defaultOrientation,
                        QString group,
                        bool defaultMaster,
                        bool defaultHeadphones);
    virtual ~TrackPlayer();
    TrackPointer getLoadedTrack() const;
    // TODO(XXX): Only exposed to let the passthrough AudioInput get
    // connected. Delete me when EngineMaster supports AudioInput assigning.
    EngineDeck* getEngineDeck() const;
    void setupEqControls();
  signals:
    void loadTrack(TrackPointer,bool bPlay=false);
    void loadTrackFailed(TrackPointer );
    void newTrackLoaded(TrackPointer );
    void unloadingTrack(TrackPointer);
  public slots:
    inline const QString&getGroup() const{return m_group;}
    inline const QString&group() const{return m_group;}
    void onLoadTrack(TrackPointer track, bool bPlay=false);
    void onFinishLoading(TrackPointer pTrackInfoObject);
    void onLoadFailed(TrackPointer pTrackInfoObject, QString reason);
    void onUnloadTrack(TrackPointer track);
    void onSetReplayGain(double replayGain);
    void onPlayToggled(double);
  private:
    const QString m_group;
    ConfigObject<ConfigValue>* m_pConfig;
    TrackPointer m_pLoadedTrack;
    // Waveform display related controls
    ControlPotmeter* m_pWaveformZoom;
    ControlObject* m_pEndOfTrack;

    ControlObjectThread* m_pLoopInPoint;
    ControlObjectThread* m_pLoopOutPoint;
    ControlObject* m_pDuration;
    ControlObjectThread* m_pBPM;
    ControlObjectThread* m_pKey;
    ControlObjectThread* m_pReplayGain;
    ControlObjectThread* m_pPlay;
    ControlObjectSlave* m_pLowFilter;
    ControlObjectSlave* m_pMidFilter;
    ControlObjectSlave* m_pHighFilter;
    ControlObjectSlave* m_pLowFilterKill;
    ControlObjectSlave* m_pMidFilterKill;
    ControlObjectSlave* m_pHighFilterKill;
    ControlObjectSlave* m_pPreGain;
    ControlObjectSlave* m_pSpeed;
    ControlObjectSlave* m_pPitchAdjust;
    QSharedPointer<EngineDeck> m_pChannel;
    bool m_replaygainPending;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(TrackPlayer::TrackLoadFlags);
Q_DECLARE_METATYPE(TrackPlayer::TrackLoadReset);
Q_DECLARE_TYPEINFO(TrackPlayer::TrackLoadReset,Q_PRIMITIVE_TYPE);

#endif // BASETRACKPLAYER_H
