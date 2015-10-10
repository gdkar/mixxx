/***************************************************************************
                          enginebuffer.h  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

_Pragma("once")
#include <QMutex>
#include <atomic>

#include "util/types.h"
#include "engine/engineobject.h"
#include "engine/sync/syncable.h"
#include "trackinfoobject.h"
#include "configobject.h"
#include "control/controlvalue.h"
#include "cachingreader.h"

class EngineChannel;
class EngineControl;
class BpmControl;
class KeyControl;
class RateControl;
class SyncControl;
class VinylControlControl;
class LoopingControl;
class ClockControl;
class CueControl;
class ReadAheadManager;
class ControlObject;
class ControlObjectSlave;
class ControlPushButton;
class ControlIndicator;
class ControlPotmeter;
class CachingReader;
class EngineBufferScale;
class EngineSync;
class EngineWorkerScheduler;
class VisualPlayPosition;
class EngineMaster;

/**
  *@author Tue and Ken Haste Andersen
*/

// Length of audio beat marks in samples
const int audioBeatMarkLen = 40;

// Temporary buffer length
const int kiTempLength = 200000;

// Rate at which the playpos slider is updated
const int kiPlaypositionUpdateRate = 10; // updates per second
// Number of kiUpdateRates that go by before we update BPM.
const int kiBpmUpdateCnt = 4; // about 2.5 updates per sec

// End of track mode constants
const int TRACK_END_MODE_STOP = 0;
const int TRACK_END_MODE_NEXT = 1;
const int TRACK_END_MODE_LOOP = 2;
const int TRACK_END_MODE_PING = 3;

const int ENGINE_RAMP_DOWN = -1;
const int ENGINE_RAMP_NONE = 0;
const int ENGINE_RAMP_UP = 1;

//const int kiRampLength = 3;

class EngineBuffer : public EngineObject {
     Q_OBJECT
  public:
    enum SyncRequestQueued {
        SYNC_REQUEST_NONE,
        SYNC_REQUEST_ENABLE,
        SYNC_REQUEST_DISABLE,
        SYNC_REQUEST_ENABLEDISABLE,
    };
    enum SeekRequest {
        NO_SEEK,
        SEEK_STANDARD,
        SEEK_EXACT,
        SEEK_PHASE
    };
    enum KeylockEngine {
        SOUNDTOUCH,
        RUBBERBAND,
        KEYLOCK_ENGINE_COUNT,
    };
    Q_ENUM(SyncRequestQueued);
    Q_ENUM(SeekRequest);
    Q_ENUM(KeylockEngine);
    EngineBuffer(QString _group, ConfigObject<ConfigValue>* _config,EngineChannel* pChannel, EngineMaster* pMixingEngine,QObject * pParent);
    virtual ~EngineBuffer();
    void bindWorkers(EngineWorkerScheduler* pWorkerScheduler);
    // Return the current rate (not thread-safe)
    double getSpeed() const;
    bool getScratching() const;
    // Returns current bpm value (not thread-safe)
    double getBpm() const;
    // Returns the BPM of the loaded track around the current position (not thread-safe)
    double getLocalBpm() const;
    // Sets pointer to other engine buffer/channel
    void setEngineMaster(EngineMaster*);
    void queueNewPlaypos(double newpos, enum SeekRequest seekType);
    void requestSyncPhase();
    void requestEnableSync(bool enabled);
    void requestSyncMode(SyncMode mode);
    // The process methods all run in the audio callback.
    void process(CSAMPLE* pOut, const int iBufferSize);
    void processSlip(int iBufferSize);
    void postProcess(const int iBufferSize);
    bool isTrackLoaded();
    TrackPointer getLoadedTrack() const;
    double getVisualPlayPos();
    double getTrackSamples();
    void collectFeatures(GroupFeatureState* pGroupFeatures) const;
    // For dependency injection of readers.
    //void setReader(CachingReader* pReader);

    // For dependency injection of scalers.
    void setScalerForTest(EngineBufferScale* pScaleVinyl,EngineBufferScale* pScaleKeylock);
    // For dependency injection of fake tracks, with an optional filebpm value.
    TrackPointer loadFakeTrack(double filebpm = 0);
    static QString getKeylockEngineName(KeylockEngine engine);
  public slots:
    void slotControlPlayRequest(double);
    void slotControlPlayFromStart(double);
    void slotControlJumpToStartAndStop(double);
    void slotControlStop(double);
    void slotControlStart(double);
    void slotControlEnd(double);
    void slotControlSeek(double);
    void slotControlSeekAbs(double);
    void slotControlSeekExact(double);
    void slotControlSlip(double);
    void slotKeylockEngineChanged(double);
    // Request that the EngineBuffer load a track. Since the process is
    // asynchronous, EngineBuffer will emit a trackLoaded signal when the load
    // has completed.
    void slotLoadTrack(TrackPointer pTrack, bool play = false);
    void slotEjectTrack(double);
  signals:
    void trackLoaded(TrackPointer pTrack);
    void trackLoadFailed(TrackPointer pTrack, QString reason);
    void trackUnloaded(TrackPointer pTrack);
  private slots:
    void slotTrackLoading();
    void slotTrackLoaded(TrackPointer pTrack,int iSampleRate, int iNumSamples);
    void slotTrackLoadFailed(TrackPointer pTrack,QString reason);
    // Fired when passthrough mode is enabled or disabled.
    void slotPassthroughChanged(double v);
  private:
    // Add an engine control to the EngineBuffer
    // must not be called outside the Constructor
    void addControl(EngineControl* pControl);
    void enableIndependentPitchTempoScaling(bool bEnable,const int iBufferSize);
    void updateIndicators(double rate, int iBufferSize);
    void hintReader(const double rate);
    void ejectTrack();
    double fractionalPlayposFromAbsolute(double absolutePlaypos);
    void doSeekFractional(double fractionalPos, enum SeekRequest seekType);
    void doSeekPlayPos(double playpos, enum SeekRequest seekType);
    // Read one buffer from the current scaler into the crossfade buffer.  Used
    // for transitioning from one scaler to another, or reseeking a scaler
    // to prevent pops.
    void readToCrossfadeBuffer(const int iBufferSize);
    // Reset buffer playpos and set file playpos.
    void setNewPlaypos(double playpos);
    void processSyncRequests();
    void processSeek();
    bool updateIndicatorsAndModifyPlay(bool newPlay);
    void verifyPlay();
    // Holds the name of the control group
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    LoopingControl* m_pLoopingControl = nullptr;
    EngineSync* m_pEngineSync         = nullptr;
    SyncControl* m_pSyncControl       = nullptr;
    VinylControlControl* m_pVinylControlControl = nullptr;
    RateControl* m_pRateControl       = nullptr;
    BpmControl* m_pBpmControl         = nullptr;
    KeyControl* m_pKeyControl         = nullptr;
    ClockControl* m_pClockControl     = nullptr;
    CueControl* m_pCueControl         = nullptr;
    QList<EngineControl*> m_engineControls;
    // The read ahead manager for EngineBufferScale's that need to read ahead
    ReadAheadManager* m_pReadAheadManager = nullptr;
    // The reader used to read audio files
    CachingReader* m_pReader              = nullptr;
    // List of hints to provide to the CachingReader
    HintVector m_hintList;
    // The current sample to play in the file.
    double m_filepos_play = 0;
    // The previous callback's speed. Used to check if the scaler parameters
    // need updating.
    double m_speed_old = 0;
    // True if the previous callback was scratching.
    bool m_scratching_old = false;
    // True if the previous callback was reverse.
    bool m_reverse_old    = false;
    // The previous callback's pitch. Used to check if the scaler parameters
    // need updating.
    double m_pitch_old    = 0;
    // The previous callback's baserate. Used to check if the scaler parameters
    // need updating.
    double m_baserate_old = 0;
    // Copy of rate_exchange, used to check if rate needs to be updated
    double m_rate_old     = 0;
    // Copy of length of file
    int m_trackSamplesOld = -1;
    // Copy of file sample rate
    int m_trackSampleRateOld = 0;
    // Mutex controlling weather the process function is in pause mode. This happens
    // during seek and loading of a new track
    QMutex m_pause;
    // Used in update of playpos slider
    int m_iSamplesCalculated = 0;
    int m_iUiSlowTick        = 0;
    // The location where the track would have been had slip not been engaged
    double m_dSlipPosition = 0;
    // Saved value of rate for slip mode
    double m_dSlipRate     = 1.0;
    // m_slipEnabled is a boolean accessed from multiple threads, so we use an atomic int.
    std::atomic<bool> m_slipEnabled{false};
    // m_bSlipEnabledProcessing is only used by the engine processing thread.
    bool m_bSlipEnabledProcessing = false;
    ControlObject* m_pTrackSamples    = nullptr;
    ControlObject* m_pTrackSampleRate = nullptr;
    ControlPushButton* m_playButton   = nullptr;
    ControlPushButton* m_playStartButton = nullptr;
    ControlPushButton* m_stopStartButton = nullptr;
    ControlPushButton* m_stopButton      = nullptr;

    ControlObject* m_fwdButton           = nullptr;
    ControlObject* m_backButton          = nullptr;
    ControlPushButton* m_pSlipButton     = nullptr;
    ControlObject* m_visualBpm           = nullptr;
    ControlObject* m_visualKey           = nullptr;
    ControlObject* m_pQuantize           = nullptr;
    ControlObject* m_pMasterRate         = nullptr;
    ControlPotmeter* m_playposSlider     = nullptr;
    ControlObjectSlave* m_pSampleRate    = nullptr;
    ControlObjectSlave* m_pKeylockEngine = nullptr;
    ControlPushButton* m_pKeylock        = nullptr;
    ControlObjectSlave* m_pPassthroughEnabled = nullptr;
    ControlPushButton* m_pEject          = nullptr;
    // Whether or not to repeat the track when at the end
    ControlPushButton* m_pRepeat         = nullptr;
    // Fwd and back controls, start and end of track control
    ControlPushButton* m_startButton     = nullptr;
    ControlPushButton* m_endButton       = nullptr;

    // Object used to perform waveform scaling (sample rate conversion).  These
    // three pointers may be reassigned depending on configuration and tests.
    EngineBufferScale* m_pScale          = nullptr;
    EngineBufferScale* m_pScaleVinyl     = nullptr;
    // The keylock engine is configurable, so it could flip flop between
    // ScaleST and ScaleRB during a single callback.
    EngineBufferScale* m_pScaleKeylock   = nullptr;
    // Object used for vinyl-style interpolation scaling of the audio
    EngineBufferScale* m_pScaleLinear = nullptr;
    // Objects used for pitch-indep time stretch (key lock) scaling of the audio
    EngineBufferScale* m_pScaleST     = nullptr;
    EngineBufferScale* m_pScaleRB     = nullptr;
    // Indicates whether the scaler has changed since the last process()
    bool m_bScalerChanged = false;
    // Indicates that dependency injection has taken place.
    bool m_bScalerOverride = false;
    std::atomic<int> m_iSeekQueued{NO_SEEK};
    std::atomic<int> m_iEnableSyncQueued{SYNC_REQUEST_NONE};
    std::atomic<int> m_iSyncModeQueued{SYNC_INVALID};
    ControlValueAtomic<double> m_queuedPosition;
    // Holds the last sample value of the previous buffer. This is used when ramping to
    // zero in case of an immediate stop of the playback
    float m_fLastSampleValue[2];
    // Is true if the previous buffer was silent due to pausing
    bool m_bLastBufferPaused = true;
    std::atomic<bool> m_iTrackLoading{false};
    bool m_bPlayAfterLoading{false};
    float m_fRampValue{0};
    int m_iRampState{ENGINE_RAMP_NONE};
    // Records the sample rate so we can detect when it changes. Initialized to
    // 0 to guarantee we see a change on the first callback.
    int m_iSampleRate{0};
    TrackPointer m_pCurrentTrack;
    std::unique_ptr<CSAMPLE[]> m_pDitherBuffer;
    unsigned int m_iDitherBufferReadIndex = 0;
    // Certain operations like seeks and engine changes need to be crossfaded
    // to eliminate clicks and pops.
    std::unique_ptr<CSAMPLE[]> m_pCrossfadeBuffer;
    bool m_bCrossfadeReady = false;
    int m_iLastBufferSize  = 0;
    QSharedPointer<VisualPlayPosition> m_visualPlayPos;
};
