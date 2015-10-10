// cuecontrol.h
// Created 11/5/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QList>
#include <QMutex>

#include "engine/enginecontrol.h"
#include "configobject.h"
#include "trackinfoobject.h"

#define NUM_HOT_CUES 8 

class ControlObject;
class ControlPushButton;
class ControlObjectSlave;
class Cue;
class ControlIndicator;

class HotcueControl : public QObject {
    Q_OBJECT
  public:
    HotcueControl(QString group, int hotcueNumber, QObject *);
    virtual ~HotcueControl();
    int getHotcueNumber() { return m_iHotcueNumber; }
    Cue* getCue() { return m_pCue; }
    void setCue(Cue* pCue) { m_pCue = pCue; }
    ControlObject* getPosition() { return m_hotcuePosition; }
    ControlObject* getEnabled() { return m_hotcueEnabled; }
    // Used for caching the preview state of this hotcue control.
    bool isPreviewing() { return m_bPreviewing; }
    void setPreviewing(bool bPreviewing) { m_bPreviewing = bPreviewing; }
    int getPreviewingPosition() { return m_iPreviewingPosition; }
    void setPreviewingPosition(int iPosition) { m_iPreviewingPosition = iPosition; }
  private slots:
    void slotHotcueSet(double v);
    void slotHotcueGoto(double v);
    void slotHotcueGotoAndPlay(double v);
    void slotHotcueGotoAndStop(double v);
    void slotHotcueActivate(double v);
    void slotHotcueActivatePreview(double v);
    void slotHotcueClear(double v);
    void slotHotcuePositionChanged(double newPosition);

  signals:
    void hotcueSet(HotcueControl* pHotcue, double v);
    void hotcueGoto(HotcueControl* pHotcue, double v);
    void hotcueGotoAndPlay(HotcueControl* pHotcue, double v);
    void hotcueGotoAndStop(HotcueControl* pHotcue, double v);
    void hotcueActivate(HotcueControl* pHotcue, double v);
    void hotcueActivatePreview(HotcueControl* pHotcue, double v);
    void hotcueClear(HotcueControl* pHotcue, double v);
    void hotcuePositionChanged(HotcueControl* pHotcue, double newPosition);
    void hotcuePlay(double v);

  private:
    ConfigKey keyForControl(int hotcue, QString name);
    QString m_group;
    int m_iHotcueNumber = -1;
    Cue* m_pCue = nullptr;
    // Hotcue state controls
    ControlObject* m_hotcuePosition = nullptr;
    ControlObject* m_hotcueEnabled = nullptr;
    // Hotcue button controls
    ControlObject* m_hotcueSet = nullptr;
    ControlObject* m_hotcueGoto = nullptr;
    ControlObject* m_hotcueGotoAndPlay = nullptr;
    ControlObject* m_hotcueGotoAndStop = nullptr;
    ControlObject* m_hotcueActivate = nullptr;
    ControlObject* m_hotcueActivatePreview = nullptr;
    ControlObject* m_hotcueClear = nullptr;
    bool m_bPreviewing = false;
    int m_iPreviewingPosition = 0;
};

class CueControl : public EngineControl {
    Q_OBJECT
  public:
    CueControl(QString group, ConfigObject<ConfigValue>* _config, QObject *);
    virtual ~CueControl();
    virtual void hintReader(HintVector* pHintList);
    bool updateIndicatorsAndModifyPlay(bool newPlay, bool playPossible);
    void updateIndicators();
    bool isTrackAtCue();
    bool getPlayFlashingAtPause();
  public slots:
    void trackLoaded(TrackPointer pTrack);
    void trackUnloaded(TrackPointer pTrack);
  private slots:
    void cueUpdated();
    void trackCuesUpdated();
    void hotcueSet(HotcueControl* pControl, double v);
    void hotcueGoto(HotcueControl* pControl, double v);
    void hotcueGotoAndPlay(HotcueControl* pControl, double v);
    void hotcueGotoAndStop(HotcueControl* pControl, double v);
    void hotcueActivate(HotcueControl* pControl, double v);
    void hotcueActivatePreview(HotcueControl* pControl, double v);
    void hotcueClear(HotcueControl* pControl, double v);
    void hotcuePositionChanged(HotcueControl* pControl, double newPosition);

    void cueSet(double v);
    void cueGoto(double v);
    void cueGotoAndPlay(double v);
    void cueGotoAndStop(double v);
    void cuePreview(double v);
    void cueCDJ(double v);
    void cueDenon(double v);
    void cueDefault(double v);
    void pause(double v);
    void playStutter(double v);

  private:
    // These methods are not thread safe, only call them when the lock is held.
    void createControls();
    void attachCue(Cue* pCue, int hotcueNumber);
    void detachCue(int hotcueNumber);
    void saveCuePoint(double cuePoint);
    bool m_bHotcueCancel     = false;
    bool m_bPreviewing       = false;
    bool m_bPreviewingHotcue = false;
    ControlObject* m_pPlayButton = nullptr;
    ControlObject* m_pStopButton = nullptr;
    int m_iCurrentlyPreviewingHotcues = -1;
    ControlObject* m_pQuantizeEnabled = nullptr;
    ControlObject* m_pNextBeat    = nullptr;
    ControlObject* m_pClosestBeat = nullptr;
    const int m_iNumHotCues = NUM_HOT_CUES;
    QList<HotcueControl*> m_hotcueControl;
    ControlObject* m_pTrackSamples = nullptr;
    ControlObject* m_pCuePoint = nullptr;
    ControlObject* m_pCueMode = nullptr;
    ControlPushButton* m_pCueSet = nullptr;
    ControlPushButton* m_pCueCDJ = nullptr;
    ControlPushButton* m_pCueDefault = nullptr;
    ControlPushButton* m_pPlayStutter = nullptr;
    ControlIndicator* m_pCueIndicator = nullptr;
    ControlIndicator* m_pPlayIndicator = nullptr;
    ControlPushButton* m_pCueGoto = nullptr;
    ControlPushButton* m_pCueGotoAndPlay = nullptr;
    ControlPushButton* m_pCueGotoAndStop = nullptr;
    ControlPushButton* m_pCuePreview = nullptr;
    ControlObjectSlave* m_pVinylControlEnabled = nullptr;
    ControlObjectSlave* m_pVinylControlMode = nullptr;
    TrackPointer m_pLoadedTrack{nullptr};
    // Tells us which controls map to which hotcue
    QMap<QObject*, int> m_controlMap;
    QMutex m_mutex;
};
