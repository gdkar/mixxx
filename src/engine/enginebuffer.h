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
#include <QAtomicInt>
#include <atomic>

#include "cachingreader.h"
#include "preferences/usersettings.h"
#include "control/controlvalue.h"
#include "engine/engineobject.h"
#include "engine/sync/syncable.h"
#include "trackinfoobject.h"
#include "util/rotary.h"
#include "util/types.h"

//for the writer
#ifdef __SCALER_DEBUG__
#include <QFile>
#include <QTextStream>
#endif

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
class ControlBeat;
class ControlTTRotary;
class ControlPotmeter;
class CachingReader;
class EngineBufferScale;
class EngineBufferScaleLinear;
class EngineBufferScaleST;
class EngineBufferScaleRubberBand;
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
  private:
  public:
    enum SyncRequestQueued {
        SYNC_REQUEST_NONE,
        SYNC_REQUEST_ENABLE,
        SYNC_REQUEST_DISABLE,
        SYNC_REQUEST_ENABLEDISABLE,
    };
    enum SeekRequest {
        SEEK_NONE = 0x00,
        SEEK_PHASE = 0x01,
        SEEK_EXACT = 0x02,
        SEEK_STANDARD = 0x03, // = (SEEK_EXACT | SEEK_PHASE)
    };
    enum KeylockEngine {
        SOUNDTOUCH,
        RUBBERBAND,
    };
    Q_ENUM(KeylockEngine);
    Q_ENUM(SyncRequestQueued);
    Q_ENUM(SeekRequest);
    Q_DECLARE_FLAGS(SeekRequests, SeekRequest);

    EngineBuffer(QString _group, UserSettingsPointer _config,EngineChannel* pChannel, EngineMaster* pMixingEngine);
    virtual ~EngineBuffer();
    void bindWorkers(EngineWorkerScheduler* pWorkerScheduler);
    // Return the current rate (not thread-safe)
    double getSpeed();
    bool getScratching();
    // Returns current bpm value (not thread-safe)
    double getBpm();
    // Returns the BPM of the loaded track around the current position (not thread-safe)
    double getLocalBpm();
    // Sets pointer to other engine buffer/channel
    void setEngineMaster(EngineMaster*);
    // Queues a new seek position. Use SEEK_EXACT or SEEK_STANDARD as seekType
    void queueNewPlaypos(double newpos, enum SeekRequest seekType);
    void requestSyncPhase();
    void requestEnableSync(bool enabled);
    void requestSyncMode(SyncMode mode);
    // The process methods all run in the audio callback.
    void process(CSAMPLE* pOut, const int iBufferSize);
    void processSlip(int iBufferSize);
    void postProcess(const int iBufferSize);
    QString getGroup();
    bool isTrackLoaded();
    TrackPointer getLoadedTrack() const;
    double getVisualPlayPos();
    double getTrackSamples();
    void collectFeatures(GroupFeatureState* pGroupFeatures) const;
    // For dependency injection of readers.
    //void setReader(CachingReader* pReader);
    static QString getKeylockEngineName(KeylockEngine engine) {
        switch (engine) {
        case SOUNDTOUCH:
            return tr("Soundtouch (faster)");
        case RUBBERBAND:
            return tr("Rubberband (better)");
        default:
            return tr("Unknown (bad value)");
        }
    }

  public slots:
    void slotControlPlayRequest(double);
    void slotControlSeek(double);
    void slotControlSeekAbs(double);
    void slotControlSeekExact(double);
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

    void positionChanged(double newpos);
    void collectingFeatures(GroupFeatureState *);
    void hintingReader(HintVector *);
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
    void processSeek(bool paused);
    bool updateIndicatorsAndModifyPlay(bool newPlay);
    void verifyPlay();
    // Holds the name of the control group
    QString m_group;
    UserSettingsPointer m_pConfig;

    LoopingControl* m_pLoopingControl{nullptr};
    EngineSync* m_pEngineSync{nullptr};
    SyncControl* m_pSyncControl{nullptr};
    VinylControlControl* m_pVinylControlControl{nullptr};
    RateControl* m_pRateControl{nullptr};
    BpmControl* m_pBpmControl{nullptr};
    KeyControl* m_pKeyControl{nullptr};
    ClockControl* m_pClockControl{nullptr};
    CueControl* m_pCueControl{nullptr};

    QList<EngineControl*> m_engineControls;

    // The read ahead manager for EngineBufferScale's that need to read ahead
    ReadAheadManager* m_pReadAheadManager{nullptr};

    // The reader used to read audio files
    CachingReader* m_pReader{nullptr};

    // List of hints to provide to the CachingReader
    HintVector m_hintList;

    // The current sample to play in the file.
    double m_filepos_play;

    // The previous callback's speed. Used to check if the scaler parameters
    // need updating.
    double m_speed_old;

    // True if the previous callback was scratching.
    bool m_scratching_old;

    // True if the previous callback was reverse.
    bool m_reverse_old;

    // The previous callback's pitch. Used to check if the scaler parameters
    // need updating.
    double m_pitch_old;

    // The previous callback's baserate. Used to check if the scaler parameters
    // need updating.
    double m_baserate_old;

    // Copy of rate_exchange, used to check if rate needs to be updated
    double m_rate_old;

    // Copy of length of file
    int m_trackSamplesOld;

    // Copy of file sample rate
    int m_trackSampleRateOld;

    // Mutex controlling weather the process function is in pause mode. This happens
    // during seek and loading of a new track
    QMutex m_pause;
    // Used in update of playpos slider
    int m_iSamplesCalculated;
    int m_iUiSlowTick;

    // The location where the track would have been had slip not been engaged
    double m_dSlipPosition;
    // Saved value of rate for slip mode
    double m_dSlipRate;
    // m_slipEnabled is a boolean accessed from multiple threads, so we use an atomic int.
    QAtomicInt m_slipEnabled;
    // m_bSlipEnabledProcessing is only used by the engine processing thread.
    bool m_bSlipEnabledProcessing;

    ControlObject* m_pTrackSamples{nullptr};
    ControlObject* m_pTrackSampleRate{nullptr};

    ControlObject* m_playing{nullptr};
    ControlPushButton* m_playButton{nullptr};
    ControlPushButton* m_playStartButton{nullptr};
    ControlPushButton* m_stopStartButton{nullptr};
    ControlPushButton* m_stopButton{nullptr};

    ControlObject* m_fwdButton{nullptr};
    ControlObject* m_backButton{nullptr};
    ControlPushButton* m_pSlipButton{nullptr};

    ControlObject* m_visualBpm{nullptr};
    ControlObject* m_visualKey{nullptr};
    ControlObject* m_pQuantize{nullptr};
    ControlObject* m_pMasterRate{nullptr};
    ControlPotmeter* m_playposSlider{nullptr};
    ControlObjectSlave* m_pSampleRate{nullptr};
    ControlObjectSlave* m_pKeylockEngine{nullptr};
    ControlPushButton* m_pKeylock{nullptr};
    // This ControlObjectSlaves is created as parent to this and deleted by
    // the Qt object tree. This helps that they are deleted by the creating
    // thread, which is required to avoid segfaults.
    ControlObjectSlave* m_pPassthroughEnabled{nullptr};
    ControlPushButton* m_pEject{nullptr};
    // Whether or not to repeat the track when at the end
    ControlPushButton* m_pRepeat{nullptr};
    // Fwd and back controls, start and end of track control
    ControlPushButton* m_startButton{nullptr};
    ControlPushButton* m_endButton{nullptr};
    // Object used to perform waveform scaling (sample rate conversion).  These
    // three pointers may be reassigned depending on configuration and tests.
    EngineBufferScale* m_pScale{nullptr};
    EngineBufferScale* m_pScaleVinyl{nullptr};
    // The keylock engine is configurable, so it could flip flop between
    // ScaleST and ScaleRB during a single callback.
    EngineBufferScale* volatile m_pScaleKeylock{nullptr};
    // Object used for vinyl-style interpolation scaling of the audio
    EngineBufferScaleLinear* m_pScaleLinear{nullptr};
    // Objects used for pitch-indep time stretch (key lock) scaling of the audio
    EngineBufferScaleST* m_pScaleST{nullptr};
    EngineBufferScaleRubberBand* m_pScaleRB{nullptr};
    // Indicates whether the scaler has changed since the last process()
    bool m_bScalerChanged;
    // Indicates that dependency injection has taken place.
    bool m_bScalerOverride;
    QAtomicInt m_iSeekQueued;
    QAtomicInt m_iSeekPhaseQueued;
    QAtomicInt m_iEnableSyncQueued;
    QAtomicInt m_iSyncModeQueued;
    ControlValueAtomic<double> m_queuedSeekPosition;

    // Holds the last sample value of the previous buffer. This is used when ramping to
    // zero in case of an immediate stop of the playback
    float m_fLastSampleValue[2];
    // Is true if the previous buffer was silent due to pausing
    bool m_bLastBufferPaused;
    QAtomicInt m_iTrackLoading;
    bool m_bPlayAfterLoading;
    float m_fRampValue;
    int m_iRampState;
    // Records the sample rate so we can detect when it changes. Initialized to
    // 0 to guarantee we see a change on the first callback.
    int m_iSampleRate;
    TrackPointer m_pCurrentTrack;
#ifdef __SCALER_DEBUG__
    QFile df;
    QTextStream writer;
#endif
    CSAMPLE* m_pDitherBuffer;
    unsigned int m_iDitherBufferReadIndex;

    // Certain operations like seeks and engine changes need to be crossfaded
    // to eliminate clicks and pops.
    CSAMPLE* m_pCrossfadeBuffer;
    bool m_bCrossfadeReady;
    int m_iLastBufferSize;
    QSharedPointer<VisualPlayPosition> m_visualPlayPos;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(EngineBuffer::SeekRequests)
