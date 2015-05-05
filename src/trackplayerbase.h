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

// Interface for not leaking implementation details of TrackPlayerBase into the
// rest of Mixxx. Also makes testing a lot easier.
class TrackPlayerBase : public QObject {
    Q_OBJECT
    Q_ENUMS(TrackLoadReset);
    Q_PROPERTY(QString group READ getGroup CONSTANT );
  public:
    // The ordering here corresponds to the ordering of the preferences combo box.
    enum TrackLoadReset {
        RESET_NONE,
        RESET_PITCH,
        RESET_PITCH_AND_SPEED,
    };
    TrackPlayerBase(QObject* pParent, const QString& group);
    virtual ~TrackPlayerBase() {}
    virtual TrackPointer getLoadedTrack() const = 0;
  public slots:
    virtual void onLoadTrack(TrackPointer pTrack, bool bPlay=false) = 0;
    virtual inline const QString &getGroup() const{return m_group;}
  signals:
    void loadTrack(TrackPointer pTrack, bool bPlay=false);
    void loadTrackFailed(TrackPointer pTrack);
    void newTrackLoaded(TrackPointer pLoadedTrack);
    void unloadingTrack(TrackPointer pAboutToBeUnloaded);

  private:
    const QString m_group;
};
Q_DECLARE_METATYPE(TrackPlayerBase::TrackLoadReset);
Q_DECLARE_TYPEINFO(TrackPlayerBase::TrackLoadReset,Q_PRIMITIVE_TYPE);
class TrackPlayerBaseImpl : public TrackPlayerBase {
    Q_OBJECT
  public:
    TrackPlayerBaseImpl(QObject* pParent,
                        ConfigObject<ConfigValue>* pConfig,
                        EngineMaster* pMixingEngine,
                        EffectsManager* pEffectsManager,
                        EngineChannel::ChannelOrientation defaultOrientation,
                        QString group,
                        bool defaultMaster,
                        bool defaultHeadphones);
    virtual ~TrackPlayerBaseImpl();
    TrackPointer getLoadedTrack() const;
    // TODO(XXX): Only exposed to let the passthrough AudioInput get
    // connected. Delete me when EngineMaster supports AudioInput assigning.
    EngineDeck* getEngineDeck() const;
    void setupEqControls();
  public slots:
    void onLoadTrack(TrackPointer track, bool bPlay=false);
    void onFinishLoading(TrackPointer pTrackInfoObject);
    void onLoadFailed(TrackPointer pTrackInfoObject, QString reason);
    void onUnloadTrack(TrackPointer track);
    void onSetReplayGain(double replayGain);
    void onPlayToggled(double);
  private:
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

#endif // BASETRACKPLAYER_H
