// loopingcontrol.h
// Created on Sep 23, 2008
// Author: asantoni, rryan

_Pragma("once")
#include <QObject>

#include "configobject.h"
#include "engine/enginecontrol.h"
#include "trackinfoobject.h"
#include "track/beats.h"

#define MINIMUM_AUDIBLE_LOOP_SIZE   300  // In samples

class ControlPushButton;
class ControlObject;

class LoopMoveControl;
class BeatJumpControl;
class BeatLoopingControl;

class LoopingControl : public EngineControl {
    Q_OBJECT
  public:
    static QList<double> getBeatSizes();
    LoopingControl(QString group, ConfigObject<ConfigValue>* _config, QObject *);
    virtual ~LoopingControl();

    // process() updates the internal state of the LoopingControl to reflect the
    // correct current sample. If a loop should be taken LoopingControl returns
    // the sample that should be seeked to. Otherwise it returns currentSample.
    virtual double process(double dRate, double currentSample, double totalSamples, int iBufferSize);
    // nextTrigger returns the sample at which the engine will be triggered to
    // take a loop, given the value of currentSample and dRate.
    virtual double nextTrigger(double dRate, double currentSample, double totalSamples, int iBufferSize);
    // getTrigger returns the sample that the engine will next be triggered to
    // loop to, given the value of currentSample and dRate.
    virtual double getTrigger(double dRate,double currentSample,double totalSamples,int iBufferSize);
    // hintReader will add to hintList hints both the loop in and loop out
    // sample, if set.
    virtual void hintReader(HintVector* pHintList);
    virtual void notifySeek(double dNewPlaypos);

  public slots:
    void slotLoopIn(double);
    void slotLoopOut(double);
    void slotLoopExit(double);
    void slotReloopExit(double);
    void slotLoopStartPos(double);
    void slotLoopEndPos(double);
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);
    void slotUpdatedTrackBeats();

    // Generate a loop of 'beats' length. It can also do fractions for a
    // beatslicing effect.
    void slotBeatLoop(double loopSize, bool keepStartPoint=false);
    void slotBeatLoopActivate(BeatLoopingControl* pBeatLoopControl);
    void slotBeatLoopActivateRoll(BeatLoopingControl* pBeatLoopControl);
    void slotBeatLoopDeactivate(BeatLoopingControl* pBeatLoopControl);
    void slotBeatLoopDeactivateRoll(BeatLoopingControl* pBeatLoopControl);

    // Jump forward or backward by beats.
    void slotBeatJump(double beats);

    // Move the loop by beats.
    void slotLoopMove(double beats);

    void slotLoopScale(double);
    void slotLoopDouble(double);
    void slotLoopHalve(double);
  private:
    void setLoopingEnabled(bool enabled);
    void clearActiveBeatLoop();
    // When a loop changes size such that the playposition is outside of the loop,
    // we can figure out the best place in the new loop to seek to maintain
    // the beat.  It will even keep multi-bar phrasing correct with 4/4 tracks.
    void seekInsideAdjustedLoop(int old_loop_in, int old_loop_out, int new_loop_in, int new_loop_out);
    ControlObject* m_pCOLoopStartPosition = nullptr;
    ControlObject* m_pCOLoopEndPosition = nullptr;
    ControlObject* m_pCOLoopEnabled = nullptr;
    ControlPushButton* m_pLoopInButton = nullptr;
    ControlPushButton* m_pLoopOutButton = nullptr;
    ControlPushButton* m_pLoopExitButton = nullptr;
    ControlPushButton* m_pReloopExitButton = nullptr;
    ControlObject* m_pCOLoopScale = nullptr;
    ControlPushButton* m_pLoopHalveButton = nullptr;
    ControlPushButton* m_pLoopDoubleButton = nullptr;
    ControlObject* m_pSlipEnabled = nullptr;
    bool m_bLoopingEnabled = false;
    bool m_bLoopRollActive = false;
    int m_iLoopEndSample = 0;
    int m_iLoopStartSample = 0;
    int m_iCurrentSample = 0;
    ControlObject* m_pQuantizeEnabled = nullptr;
    ControlObject* m_pNextBeat = nullptr;
    ControlObject* m_pClosestBeat = nullptr;
    ControlObject* m_pTrackSamples = nullptr;
    BeatLoopingControl* m_pActiveBeatLoop = nullptr;
    // Base BeatLoop Control Object.
    ControlObject* m_pCOBeatLoop = nullptr;
    // Different sizes for Beat Loops/Seeks.
    static double s_dBeatSizes[];
    // Array of BeatLoopingControls, one for each size.
    QList<BeatLoopingControl*> m_beatLoops;
    ControlObject* m_pCOBeatJump = nullptr;
    QList<BeatJumpControl*> m_beatJumps;
    ControlObject* m_pCOLoopMove = nullptr;
    QList<LoopMoveControl*> m_loopMoves;
    TrackPointer m_pTrack{nullptr};
    BeatsPointer m_pBeats{nullptr};
};
// Class for handling loop moves of a set size. This allows easy access from
// skins.
class LoopMoveControl : public QObject {
    Q_OBJECT
  public:
    LoopMoveControl(QString group, double size, QObject *);
    virtual ~LoopMoveControl();
  signals:
    void loopMove(double beats);
  public slots:
    void slotMoveForward(double value);
    void slotMoveBackward(double value);
  private:
    double m_dLoopMoveSize = 0;
    ControlPushButton* m_pMoveForward = nullptr;
    ControlPushButton* m_pMoveBackward = nullptr;
};
// Class for handling beat jumps of a set size. This allows easy access from
// skins.
class BeatJumpControl : public QObject {
    Q_OBJECT
  public:
    BeatJumpControl(QString group, double size, QObject *);
    virtual ~BeatJumpControl();
  signals:
    void beatJump(double beats);
  public slots:
    void slotJumpForward(double value);
    void slotJumpBackward(double value);
  private:
    double m_dBeatJumpSize;
    ControlPushButton* m_pJumpForward = nullptr;
    ControlPushButton* m_pJumpBackward= nullptr;
};
// Class for handling beat loops of a set size. This allows easy access from
// skins.
class BeatLoopingControl : public QObject {
    Q_OBJECT
  public:
    BeatLoopingControl(QString group, double size, QObject *);
    virtual ~BeatLoopingControl();
    void activate();
    void deactivate();
    double getSize() const;
  public slots:
    void slotLegacy(double value);
    void slotActivate(double value);
    void slotActivateRoll(double value);
    void slotToggle(double value);
  signals:
    void activateBeatLoop(BeatLoopingControl*);
    void deactivateBeatLoop(BeatLoopingControl*);
    void activateBeatLoopRoll(BeatLoopingControl*);
    void deactivateBeatLoopRoll(BeatLoopingControl*);
  private:
    double m_dBeatLoopSize = 0.0;
    bool m_bActive = false;
    ControlPushButton* m_pLegacy = nullptr;
    ControlPushButton* m_pActivate = nullptr;
    ControlPushButton* m_pActivateRoll = nullptr;
    ControlPushButton* m_pToggle = nullptr;
    ControlObject* m_pEnabled = nullptr;
};
