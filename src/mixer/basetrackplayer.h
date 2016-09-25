#ifndef MIXER_BASETRACKPLAYER_H
#define MIXER_BASETRACKPLAYER_H

#include <QObject>
#include <QScopedPointer>
#include <QString>

#include "preferences/usersettings.h"
#include "engine/enginechannel.h"
#include "engine/enginedeck.h"
#include "mixer/baseplayer.h"
#include "track/track.h"

class EngineMaster;
class ControlObject;
class ControlPotmeter;
class ControlProxy;
class EffectsManager;


class TrackPlayer : public BasePlayer {
    Q_OBJECT
  public:
    enum TrackLoadReset {
        RESET_NONE,
        RESET_PITCH,
        RESET_PITCH_AND_SPEED,
        RESET_SPEED
    };
    Q_ENUM(TrackLoadReset);
    TrackPlayer(QObject* pParent,
                    UserSettingsPointer pConfig,
                    EngineMaster* pMixingEngine,
                    EffectsManager* pEffectsManager,
                    EngineChannel::ChannelOrientation defaultOrientation,
                    const QString& group,
                    bool defaultMaster,
                    bool defaultHeadphones);
    virtual ~TrackPlayer();


    // TODO(XXX): Only exposed to let the passthrough AudioInput get
    // connected. Delete me when EngineMaster supports AudioInput assigning.
    EngineDeck* getEngineDeck() const;

    void setupEqControls();
  signals:
    void newTrackLoaded(TrackPointer pLoadedTrack);
    void loadingTrack(TrackPointer pNewTrack, TrackPointer pOldTrack);
    void playerEmpty();
    void noPassthroughInputConfigured();
    void noVinylControlInputConfigured();

  public slots:
    TrackPointer getLoadedTrack() const;
    void slotLoadTrack(TrackPointer pTrack, bool bPlay=false) ;
    void slotTrackLoaded(TrackPointer pNewTrack, TrackPointer pOldTrack);
    void slotLoadFailed(TrackPointer pTrack, QString reason);
    void slotSetReplayGain(mixxx::ReplayGain replayGain);
    void slotPlayToggled(double);

  private slots:
    void slotPassthroughEnabled(double v);
    void slotVinylControlEnabled(double v);

  private:
    void setReplayGain(double value);

    UserSettingsPointer m_pConfig;
    EngineMaster* m_pEngineMaster;
    TrackPointer m_pLoadedTrack;

    // Waveform display related controls
    ControlPotmeter* m_pWaveformZoom;
    ControlObject* m_pEndOfTrack;

    ControlProxy * m_pLoopInPoint;
    ControlProxy * m_pLoopOutPoint;
    ControlObject* m_pDuration;

    // TODO() these COs are reconnected during runtime
    // This may lock the engine
    ControlProxy* m_pBPM;
    ControlProxy* m_pKey;

    ControlProxy* m_pReplayGain;
    ControlProxy* m_pPlay;
    ControlObject * m_pLowFilter;
    ControlObject * m_pMidFilter;
    ControlObject * m_pHighFilter;
    ControlObject * m_pLowFilterKill;
    ControlObject *  m_pMidFilterKill;
    ControlObject * m_pHighFilterKill;
    ControlObject * m_pPreGain;
    ControlObject * m_pRateSlider;
    ControlProxy* m_pPitchAdjust;
    ControlProxy* m_pInputConfigured;
    ControlProxy* m_pPassthroughEnabled;
    ControlProxy* m_pVinylControlEnabled;
    ControlProxy* m_pVinylControlStatus;
    EngineDeck* m_pChannel;

    bool m_replaygainPending;
};

inline TrackPlayer *makeDeck(
    QObject *pParent,
    UserSettingsPointer pConfig,
    EngineMaster *pMixingEngine,
    EffectsManager *pEffectsManager,
    EngineChannel::ChannelOrientation defaultOrientation,
    const QString &group)
{
    return new TrackPlayer(pParent,pConfig,pMixingEngine,pEffectsManager,defaultOrientation,group,true,false);
}
inline TrackPlayer *makeSampler(
    QObject *pParent,
    UserSettingsPointer pConfig,
    EngineMaster *pMixingEngine,
    EffectsManager *pEffectsManager,
    EngineChannel::ChannelOrientation defaultOrientation,
    const QString &group)
{
    return new TrackPlayer(pParent,pConfig,pMixingEngine,pEffectsManager,defaultOrientation,group,true,false);
}
inline TrackPlayer *makePreviewDeck(
    QObject *pParent,
    UserSettingsPointer pConfig,
    EngineMaster *pMixingEngine,
    EffectsManager *pEffectsManager,
    EngineChannel::ChannelOrientation defaultOrientation,
    const QString &group)
{
    return new TrackPlayer(pParent,pConfig,pMixingEngine,pEffectsManager,defaultOrientation,group,false,true);
}
#endif // MIXER_BASETRACKPLAYER_H
