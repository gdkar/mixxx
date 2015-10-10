_Pragma("once")
#include <QObject>
#include <QtPlugin>
#include <QString>

#include "engine/sync/clock.h"
#include "engine/sync/syncable.h"
#include "engine/enginechannel.h"

class ControlObject;
class ControlLinPotmeter;
class ControlPushButton;
class EngineSync;

class InternalClock : public QObject, public Clock, public Syncable {
    Q_OBJECT
    Q_INTERFACES(Clock);
    Q_INTERFACES(Syncable);
  public:
    InternalClock(const char* pGroup, SyncableListener* pEngineSync, QObject *pParent=nullptr);
    virtual ~InternalClock();
    virtual QString getGroup() const;
    virtual EngineChannel* getChannel() const;
    virtual void notifySyncModeChanged(SyncMode mode);
    virtual void notifyOnlyPlayingSyncable();
    virtual void requestSyncPhase();
    virtual SyncMode getSyncMode() const;
    // The clock is always "playing" in a sense but this specifically refers to
    // decks so always return false.
    virtual bool isPlaying() const;
    virtual double getBeatDistance() const;
    virtual void setMasterBeatDistance(double beatDistance);
    virtual double getBaseBpm() const;
    virtual void setMasterBaseBpm(double);
    virtual void setMasterBpm(double bpm);
    virtual double getBpm() const;
    virtual void setInstantaneousBpm(double bpm);
    virtual void setMasterParams(double beatDistance, double baseBpm, double bpm);
    virtual void onCallbackStart(int sampleRate, int bufferSize);
    virtual void onCallbackEnd(int sampleRate, int bufferSize);
  public  slots:
    virtual void slotBpmChanged(double bpm);
    virtual void slotBeatDistanceChanged(double beat_distance);
    virtual void slotSyncMasterEnabledChangeRequest(double state);
  private:
    void updateBeatLength(int sampleRate, double bpm);
    QString m_group;
    SyncableListener* m_pEngineSync = nullptr;
    ControlLinPotmeter* m_pClockBpm;
    ControlObject* m_pClockBeatDistance;
    ControlPushButton* m_pSyncMasterEnabled;
    SyncMode m_mode;
    int m_iOldSampleRate = 0;
    double m_dOldBpm = 0;
    // The internal clock rate is stored in terms of samples per beat.
    // Fractional values are allowed.
    double m_dBeatLength = 0;
    // The current number of frames accumulated since the last beat (e.g. beat
    // distance is m_dClockPosition / m_dBeatLength).
    double m_dClockPosition = 0;
};
