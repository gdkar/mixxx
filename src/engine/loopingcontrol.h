// loopingcontrol.h
// Created on Sep 23, 2008
// Author: asantoni, rryan

#ifndef LOOPINGCONTROL_H
#define LOOPINGCONTROL_H

#include <QObject>

#include "configobject.h"
#include "engine/enginecontrol.h"
#include "track/trackinfoobject.h"
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

    LoopingControl(QString group, ConfigObject<ConfigValue>* _config, QObject *pParent=0);
    virtual ~LoopingControl();

    // process() updates the internal state of the LoopingControl to reflect the
    // correct current sample. If a loop should be taken LoopingControl returns
    // the sample that should be seeked to. Otherwise it returns currentSample.
    virtual double process(const double dRate,
                   const double currentSample,
                   const double totalSamples,
                   const int iBufferSize);

    // nextTrigger returns the sample at which the engine will be triggered to
    // take a loop, given the value of currentSample and dRate.
    virtual double nextTrigger(const double dRate,
                       const double currentSample,
                       const double totalSamples,
                       const int iBufferSize);

    // getTrigger returns the sample that the engine will next be triggered to
    // loop to, given the value of currentSample and dRate.
    virtual double getTrigger(const double dRate,
                      const double currentSample,
                      const double totalSamples,
                      const int iBufferSize);

    // hintReader will add to hintList hints both the loop in and loop out
    // sample, if set.
    virtual void hintReader(HintVector* pHintList);
    virtual void notifySeek(double dNewPlaypos);
  public slots:
    void onLoopIn(double);
    void onLoopOut(double);
    void onLoopExit(double);
    void onReloopExit(double);
    void onLoopStartPos(double);
    void onLoopEndPos(double);
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);
    void onUpdatedTrackBeats();

    // Generate a loop of 'beats' length. It can also do fractions for a
    // beatslicing effect.
    void onBeatLoop(double loopSize, bool keepStartPoint=false);
    void onBeatLoopActivate(BeatLoopingControl* pBeatLoopControl);
    void onBeatLoopActivateRoll(BeatLoopingControl* pBeatLoopControl);
    void onBeatLoopDeactivate(BeatLoopingControl* pBeatLoopControl);
    void onBeatLoopDeactivateRoll(BeatLoopingControl* pBeatLoopControl);
    // Jump forward or backward by beats.
    void onBeatJump(double beats);
    // Move the loop by beats.
    void onLoopMove(double beats);
    void onLoopScale(double);
    void onLoopDouble(double);
    void onLoopHalve(double);
  private:
    void setLoopingEnabled(bool enabled);
    void clearActiveBeatLoop();
    // When a loop changes size such that the playposition is outside of the loop,
    // we can figure out the best place in the new loop to seek to maintain
    // the beat.  It will even keep multi-bar phrasing correct with 4/4 tracks.
    void seekInsideAdjustedLoop(int old_loop_in, int old_loop_out,
                                int new_loop_in, int new_loop_out);
    ControlObject* m_pCOLoopStartPosition;
    ControlObject* m_pCOLoopEndPosition;
    ControlObject* m_pCOLoopEnabled;
    ControlPushButton* m_pLoopInButton;
    ControlPushButton* m_pLoopOutButton;
    ControlPushButton* m_pLoopExitButton;
    ControlPushButton* m_pReloopExitButton;
    ControlObject* m_pCOLoopScale;
    ControlPushButton* m_pLoopHalveButton;
    ControlPushButton* m_pLoopDoubleButton;
    ControlObject* m_pSlipEnabled;

    bool m_bLoopingEnabled;
    bool m_bLoopRollActive;
    int m_iLoopEndSample;
    int m_iLoopStartSample;
    int m_iCurrentSample;
    ControlObject* m_pQuantizeEnabled;
    ControlObject* m_pNextBeat;
    ControlObject* m_pClosestBeat;
    ControlObject* m_pTrackSamples;
    BeatLoopingControl* m_pActiveBeatLoop;

    // Base BeatLoop Control Object.
    ControlObject* m_pCOBeatLoop;
    // Different sizes for Beat Loops/Seeks.
    static double s_dBeatSizes[];
    // Array of BeatLoopingControls, one for each size.
    QList<BeatLoopingControl*> m_beatLoops;

    ControlObject* m_pCOBeatJump;
    QList<BeatJumpControl*> m_beatJumps;

    ControlObject* m_pCOLoopMove;
    QList<LoopMoveControl*> m_loopMoves;

    TrackPointer m_pTrack;
    BeatsPointer m_pBeats;
};

// Class for handling loop moves of a set size. This allows easy access from
// skins.
class LoopMoveControl : public QObject {
    Q_OBJECT
  public:
    LoopMoveControl(QString group, double size);
    virtual ~LoopMoveControl();

  signals:
    void loopMove(double beats);

  public slots:
    void onMoveForward(double value);
    void onMoveBackward(double value);

  private:
    double m_dLoopMoveSize;
    ControlPushButton* m_pMoveForward;
    ControlPushButton* m_pMoveBackward;
};

// Class for handling beat jumps of a set size. This allows easy access from
// skins.
class BeatJumpControl : public QObject {
    Q_OBJECT
  public:
    BeatJumpControl(QString group, double size);
    virtual ~BeatJumpControl();

  signals:
    void beatJump(double beats);

  public slots:
    void onJumpForward(double value);
    void onJumpBackward(double value);

  private:
    double m_dBeatJumpSize;
    ControlPushButton* m_pJumpForward;
    ControlPushButton* m_pJumpBackward;
};

// Class for handling beat loops of a set size. This allows easy access from
// skins.
class BeatLoopingControl : public QObject {
    Q_OBJECT
  public:
    BeatLoopingControl(QString group, double size);
    virtual ~BeatLoopingControl();

    void activate();
    void deactivate();
    inline double getSize() {
        return m_dBeatLoopSize;
    }
  public slots:
    void onLegacy(double value);
    void onActivate(double value);
    void onActivateRoll(double value);
    void onToggle(double value);

  signals:
    void activateBeatLoop(BeatLoopingControl*);
    void deactivateBeatLoop(BeatLoopingControl*);
    void activateBeatLoopRoll(BeatLoopingControl*);
    void deactivateBeatLoopRoll(BeatLoopingControl*);

  private:
    double m_dBeatLoopSize;
    bool m_bActive;
    ControlPushButton* m_pLegacy;
    ControlPushButton* m_pActivate;
    ControlPushButton* m_pActivateRoll;
    ControlPushButton* m_pToggle;
    ControlObject* m_pEnabled;
};

#endif /* LOOPINGCONTROL_H */
