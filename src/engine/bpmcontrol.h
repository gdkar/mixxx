_Pragma("once")
#include "engine/enginecontrol.h"
#include "engine/sync/syncable.h"
#include "util/tapfilter.h"

class ControlObject;
class EngineBuffer;
class SyncControl;

class BpmControl : public EngineControl {
    Q_OBJECT

  public:
    BpmControl(QString group, ConfigObject<ConfigValue>* _config);
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
    static double shortestPercentageChange(double current_percentage,
                                           double target_percentage);

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
    ControlObject* m_pPlayButton;
    ControlObject* m_pReverseButton;
    ControlObject* m_pRateSlider;
    ControlObject* m_pQuantize;
    ControlObject* m_pRateRange;
    ControlObject* m_pRateDir;

    // ControlObjects that come from QuantizeControl
    QScopedPointer<ControlObject> m_pNextBeat;
    QScopedPointer<ControlObject> m_pPrevBeat;
    QScopedPointer<ControlObject> m_pClosestBeat;

    // ControlObjects that come from LoopingControl
    ControlObject* m_pLoopEnabled;
    ControlObject* m_pLoopStartPosition;
    ControlObject* m_pLoopEndPosition;

    ControlObject* m_pVCEnabled;

    // The current loaded file's detected BPM
    ControlObject* m_pFileBpm;
    // The average bpm around the current playposition;
    ControlObject* m_pLocalBpm;
    ControlObject* m_pAdjustBeatsFaster;
    ControlObject* m_pAdjustBeatsSlower;
    ControlObject* m_pTranslateBeatsEarlier;
    ControlObject* m_pTranslateBeatsLater;

    // The current effective BPM of the engine
    ControlObject* m_pEngineBpm;

    // Used for bpm tapping from GUI and controllers
    ControlObject* m_pButtonTap;

    // Button for sync'ing with the other EngineBuffer
    ControlObject* m_pButtonSync;
    ControlObject* m_pButtonSyncPhase;
    ControlObject* m_pButtonSyncTempo;

    // Button that translates the beats so the nearest beat is on the current
    // playposition.
    ControlObject* m_pTranslateBeats;
    // Button that translates beats to match another playing deck
    ControlObject* m_pBeatsTranslateMatchAlignment;

    double m_dPreviousSample;

    // Master Sync objects and values.
    ControlObject* m_pSyncMode;
    ControlObject* m_pThisBeatDistance;
    double m_dSyncTargetBeatDistance;
    double m_dSyncInstantaneousBpm;
    double m_dLastSyncAdjustment;
    bool m_resetSyncAdjustment;
    double m_dUserOffset;

    TapFilter m_tapFilter;

    TrackPointer m_pTrack;
    BeatsPointer m_pBeats;

    QString m_sGroup;
};

