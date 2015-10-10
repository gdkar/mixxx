_Pragma("once")
#include "engine/enginecontrol.h"
#include "engine/sync/syncable.h"
#include "util/tapfilter.h"

class ControlObject;
class ControlLinPotmeter;
class ControlObjectSlave;
class ControlPushButton;
class EngineBuffer;
class SyncControl;

class BpmControl : public EngineControl {
    Q_OBJECT
  public:
    BpmControl(QString group, ConfigObject<ConfigValue>* _config, QObject*);
    virtual ~BpmControl();
    double getBpm() const;
    double getLocalBpm() const; 
    // When in master sync mode, ratecontrol calls calcSyncedRate to figure out
    // how fast the track should play back.  The returned rate is usually just
    // the correct pitch to match bpms.  The usertweak argument represents
    // how much the user is nudging the pitch to get two tracks into sync, and
    // that value is added to the rate by bpmcontrol.  The rate may be
    // further adjusted if bpmcontrol discovers that the tracks have fallen
    // out of sync.
    double calcSyncedRate(double userTweak);
    // Get the phase offset from the specified position.
    double getPhaseOffset(double reference_position);
    double getBeatDistance(double dThisPosition) const;
    double getPreviousSample() const;

    void setCurrentSample(double dCurrentSample, double dTotalSamples);
    double process(double dRate,
                   double dCurrentSample,
                   double dTotalSamples,
                   int iBufferSize);
    void setTargetBeatDistance(double beatDistance);
    void setInstantaneousBpm(double instantaneousBpm);
    void resetSyncAdjustment();
    double updateLocalBpm();
    double updateBeatDistance();
    void collectFeatures(GroupFeatureState* pGroupFeatures) const;
    // Calculates contextual information about beats: the previous beat, the
    // next beat, the current beat length, and the beat ratio (how far dPosition
    // lies within the current beat). Returns false if a previous or next beat
    // does not exist. NULL arguments are safe and ignored.
    static bool getBeatContext(const BeatsPointer& pBeats,
                               double dPosition,
                               double* dpPrevBeat,
                               double* dpNextBeat,
                               double* dpBeatLength,
                               double* dpBeatPercentage);
    // Alternative version that works if the next and previous beat positions
    // are already known.
    static bool getBeatContextNoLookup(
                               double dPosition,
                               double dPrevBeat,
                               double dNextBeat,
                               double* dpBeatLength,
                               double* dpBeatPercentage);
    // Returns the shortest change in percentage needed to achieve
    // target_percentage.
    // Example: shortestPercentageChange(0.99, 0.01) == 0.02
    static double shortestPercentageChange(double current_percentage, double target_percentage);
  public slots:
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);
  private slots:
    void slotSetEngineBpm(double);
    void slotFileBpmChanged(double);
    void slotAdjustBeatsFaster(double);
    void slotAdjustBeatsSlower(double);
    void slotTranslateBeatsEarlier(double);
    void slotTranslateBeatsLater(double);
    void slotControlPlay(double);
    void slotControlBeatSync(double);
    void slotControlBeatSyncPhase(double);
    void slotControlBeatSyncTempo(double);
    void slotTapFilter(double,int);
    void slotBpmTap(double);
    void slotAdjustRateSlider();
    void slotUpdatedTrackBeats();
    void slotBeatsTranslate(double);
    void slotBeatsTranslateMatchAlignment(double);

  private:
    SyncMode getSyncMode() const; 
    bool syncTempo();
    double calcSyncAdjustment(double my_percentage, bool userTweakingSync);
    friend class SyncControl;
    // ControlObjects that come from EngineBuffer
    ControlObjectSlave* m_pPlayButton     = nullptr;
    ControlObjectSlave* m_pReverseButton  = nullptr;
    ControlObjectSlave* m_pRateSlider     = nullptr;
    ControlObject* m_pQuantize            = nullptr;
    ControlObjectSlave* m_pRateRange      = nullptr;
    ControlObjectSlave* m_pRateDir        = nullptr;
    // ControlObjects that come from QuantizeControl
    QScopedPointer<ControlObjectSlave> m_pNextBeat;
    QScopedPointer<ControlObjectSlave> m_pPrevBeat;
    QScopedPointer<ControlObjectSlave> m_pClosestBeat;
    // ControlObjects that come from LoopingControl
    ControlObjectSlave* m_pLoopEnabled          = nullptr;
    ControlObjectSlave* m_pLoopStartPosition    = nullptr;
    ControlObjectSlave* m_pLoopEndPosition      = nullptr;
    ControlObjectSlave* m_pVCEnabled            = nullptr;
    // The current loaded file's detected BPM
    ControlObject* m_pFileBpm                   = nullptr;
    // The average bpm around the current playposition;
    ControlObject* m_pLocalBpm                  = nullptr;
    ControlPushButton* m_pAdjustBeatsFaster     = nullptr;
    ControlPushButton* m_pAdjustBeatsSlower     = nullptr;
    ControlPushButton* m_pTranslateBeatsEarlier = nullptr;
    ControlPushButton* m_pTranslateBeatsLater   = nullptr;
    // The current effective BPM of the engine
    ControlLinPotmeter* m_pEngineBpm      = nullptr;
    // Used for bpm tapping from GUI and controllers
    ControlPushButton* m_pButtonTap       = nullptr;
    // Button for sync'ing with the other EngineBuffer
    ControlPushButton* m_pButtonSync      = nullptr;
    ControlPushButton* m_pButtonSyncPhase = nullptr;
    ControlPushButton* m_pButtonSyncTempo = nullptr;
    // Button that translates the beats so the nearest beat is on the current
    // playposition.
    ControlPushButton* m_pTranslateBeats = nullptr;
    // Button that translates beats to match another playing deck
    ControlPushButton* m_pBeatsTranslateMatchAlignment = nullptr;
    double m_dPreviousSample = 0;
    // Master Sync objects and values.
    ControlObject* m_pSyncMode = nullptr;
    ControlObjectSlave* m_pThisBeatDistance = nullptr;
    double m_dSyncTargetBeatDistance = 0;
    double m_dSyncInstantaneousBpm = 0;
    double m_dLastSyncAdjustment = 1.0;
    bool m_resetSyncAdjustment = false;
    double m_dUserOffset = 0;
    TapFilter m_tapFilter;
    TrackPointer m_pTrack{nullptr};
    BeatsPointer m_pBeats{nullptr};
    QString m_sGroup;
};

