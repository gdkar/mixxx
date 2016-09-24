#ifndef SYNCABLE_H
#define SYNCABLE_H

#include <QString>

class EngineChannel;

enum SyncMode {
    SYNC_INVALID = -1,
    SYNC_NONE = 0,
    SYNC_FOLLOWER = 1,
    SYNC_MASTER = 2,
    SYNC_NUM_MODES
};

constexpr SyncMode syncModeFromDouble(double value) {
    // msvs does not allow to cast from double to an enum
    auto mode = static_cast<SyncMode>(int(value));
    if (mode >= SYNC_NUM_MODES || mode < 0)
        return SYNC_NONE;
    return mode;
}

class Syncable {
  public:
    virtual ~Syncable() {}
//    virtual ~Syncable() { }
    virtual const QString& getGroup() const = 0;
    virtual EngineChannel* getChannel() const = 0;
    // Notify a Syncable that their mode has changed. The Syncable must record
    // this mode and return the latest mode received via notifySyncModeChanged
    // in response to getMode().
    virtual void notifySyncModeChanged(SyncMode mode) = 0;
    // Notify a Syncable that it is now the only currently-playing syncable.
    virtual void notifyOnlyPlayingSyncable() = 0;
    // Notify a Syncable that they should sync phase.
    virtual void requestSyncPhase() = 0;
    // Must NEVER return a mode that was not set directly via
    // notifySyncModeChanged.
    virtual SyncMode getSyncMode() const = 0;
    // Only relevant for player Syncables.
    virtual bool isPlaying() const = 0;
    // Gets the current speed of the syncable in bpm (bpm * rate slider), doesn't
    // include scratch or FF/REW values.
    virtual double getBpm() const = 0;
    virtual double getBeatDistance() const = 0;
    // Gets the speed of the syncable if it was playing at 1.0 rate.
    virtual double getBaseBpm() const = 0;
    // The following functions are used to tell syncables about the state of the
    // current Sync Master.
    // Must never result in a call to
    // SyncableListener::notifyBeatDistanceChanged or signal loops could occur.
    virtual void setMasterBeatDistance(double beatDistance) = 0;
    // Reports what the bpm of the master would be if it were playing back at
    // a rate of 1.0x.  This is used by syncables to decide if they should
    // match rates at x2 or /2 speed.  If we were to use the regular BPM, the
    // change of a rate slider might suddenly change the sync multiplier.
    virtual void setMasterBaseBpm(double) = 0;
    // Must never result in a call to SyncableListener::notifyBpmChanged or
    // signal loops could occur.
    virtual void setMasterBpm(double bpm) = 0;
    // Combines the above three calls into one, since they are often set
    // simultaneously.  Avoids redundant recalculation that would occur by
    // using the three calls separately.
    virtual void setMasterParams(double beatDistance, double baseBpm, double bpm) = 0;
    // Must never result in a call to
    // SyncableListener::notifyInstantaneousBpmChanged or signal loops could
    // occur.
    virtual void setInstantaneousBpm(double bpm) = 0;
};
Q_DECLARE_INTERFACE(Syncable, "org.mixxx.interfaces.syncable/0.1");

#endif /* SYNCABLE_H */
