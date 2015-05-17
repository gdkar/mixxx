#ifndef BASETRACKPLAYER_H
#define BASETRACKPLAYER_H

#include "configobject.h"
#include "control/atom.h"
#include "trackinfoobject.h"
#include "engine/enginechannel.h"
#include "engine/enginedeck.h"

class EngineMaster;
class ControlObject;
class ControlPotmeter;
class ControlObjectThread;
class ControlObjectSlave;
class AnalyserQueue;
class EffectsManager;

class TrackPlayer: public QObject{
    Q_OBJECT;
    Q_PROPERTY(QString group READ getGroup CONSTANT);
    Q_PROPERTY(EngineChannel::ChannelOrientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged);
    Q_PROPERTY(CVPtr waveformZoom READ waveformZoom CONSTANT);
    Q_PROPERTY(CVPtr bpm READ bpm );
    Q_PROPERTY(CVPtr key READ key CONSTANT);
    Q_PROPERTY(CVPtr replayGain READ replayGain CONSTANT);
    Q_PROPERTY(CVPtr preGain READ preGain CONSTANT);
    Q_PROPERTY(CVPtr speed READ speed CONSTANT);
    Q_PROPERTY(CVPtr pitch READ pitch CONSTANT);
    Q_PROPERTY(CVPtr duration READ duration CONSTANT );
    Q_PROPERTY(CVPtr position READ position CONSTANT );

  public:
    enum TrackLoadReset {
        RESET_NONE,
        RESET_PITCH,
        RESET_PITCH_AND_SPEED,
    };
    explicit TrackPlayer(ConfigObject<ConfigValue>* pConfig,
                        QString group,
                        EngineMaster* pMixingEngine,
                        QObject* pParent);
    virtual ~TrackPlayer();
    // TODO(XXX): Only exposed to let the passthrough AudioInput get
    // connected. Delete me when EngineMaster supports AudioInput assigning.
    EngineDeck* getEngineDeck() const;
    void setupEqControls();
  public slots:
    virtual void slotLoadTrack(TrackPointer track, bool bPlay=false);
    virtual void slotFinishLoading(TrackPointer pTrackInfoObject);
    virtual void slotLoadFailed(TrackPointer pTrackInfoObject, QString reason);
    virtual void slotUnloadTrack(TrackPointer track);
    virtual void slotSetReplayGain(double replayGain);
    virtual void slotPlayToggled(double);
    virtual TrackPointer getLoadedTrack() const ;
    inline const QString& getGroup() {return m_group;}
    virtual EngineChannel::ChannelOrientation orientation()const{return m_pChannel->orientation();}
    virtual void setOrientation(EngineChannel::ChannelOrientation o){m_pChannel->setOrientation(o);}
    virtual CVPtr &waveformZoom() {return m_waveformZoom;}
    virtual CVPtr &bpm(){return m_bpm;}
    virtual CVPtr &key(){return m_key;}
    virtual CVPtr &replayGain(){return m_replayGain;}
    virtual CVPtr &preGain(){return m_preGain;}
    virtual CVPtr &speed(){return m_speed;}
    virtual CVPtr &pitch(){return m_pitch;}
    virtual CVPtr &position(){return m_position;}
    virtual CVPtr &duration(){return m_duration;}
  signals:
    void loadTrack(TrackPointer pTrack, bool bPlay=false);
    void loadTrackFailed(TrackPointer pTrack);
    void newTrackLoaded(TrackPointer pLoadedTrack);
    void unloadingTrack(TrackPointer pAboutToBeUnloaded);
    void orientationChanged(EngineChannel::ChannelOrientation);
  private:
    const QString         m_group;
    ConfigObject<ConfigValue>* m_pConfig;
    CVPtr                m_waveformZoom;
    CVPtr                m_bpm;
    CVPtr                m_key;
    CVPtr                m_replayGain;
    CVPtr                m_preGain;
    CVPtr                m_speed;
    CVPtr                m_pitch;
    CVPtr                m_duration;
    CVPtr                m_position;
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
    EngineDeck* m_pChannel;

    bool m_replaygainPending;
};

#endif // BASETRACKPLAYER_H
