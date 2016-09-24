/***************************************************************************
                          enginesync.h  -  master sync control for
                          maintaining beatmatching amongst n decks
                             -------------------
    begin                : Mon Mar 12 2012
    copyright            : (C) 2012 by Owen Williams
    email                : owilliams@mixxx.org
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef ENGINESYNC_H
#define ENGINESYNC_H

#include "preferences/usersettings.h"
#include "engine/sync/syncable.h"
//#include "engine/sync/basesyncablelistener.h"
class InternalClock;

class EngineSync : public QObject {
    Q_OBJECT
  public:
    explicit EngineSync(QObject *pParent = nullptr);
    explicit EngineSync(UserSettingsPointer pConfig, QObject *pParent = nullptr);
    virtual ~EngineSync();
    // Used by Syncables to tell EngineSync it wants to be enabled in a
    // specific mode. If the state change is accepted, EngineSync calls
    // Syncable::notifySyncModeChanged.
    void requestSyncMode(Syncable* pSyncable, SyncMode state);
    // Used by Syncables to tell EngineSync it wants to be enabled in any mode
    // (master/follower).
    void requestEnableSync(Syncable* pSyncable, bool enabled);
    // Syncables notify EngineSync directly about various events. EngineSync
    // does not have a say in whether these succeed or not, they are simply
    // notifications.
    void notifyBpmChanged(Syncable* pSyncable, double bpm, bool fileChanged=false);
    void notifyInstantaneousBpmChanged(Syncable* pSyncable, double bpm);
    void notifyBeatDistanceChanged(Syncable* pSyncable, double beatDistance);
    void notifyPlaying(Syncable* pSyncable, bool playing);
    void notifyScratching(Syncable* pSyncable, bool scratching);
    void notifyTrackLoaded(Syncable* pSyncable, double suggested_bpm);

    // Used to pick a sync target for non-master-sync mode.
    EngineChannel* pickNonSyncSyncTarget(EngineChannel* pDontPick) const;
    // Used to test whether changing the rate of a Syncable would change the rate
    // of other Syncables that are playing
    bool otherSyncedPlaying(const QString& group);
    void addSyncableDeck(Syncable* pSyncable);
    EngineChannel* getMaster() const;
    void onCallbackStart(int sampleRate, int bufferSize);
    void onCallbackEnd(int sampleRate, int bufferSize);
    // Only for testing. Do not use.
    Syncable* getSyncableForGroup(const QString& group);
    Syncable* getMasterSyncable() {
        return m_pMasterSyncable;
    }
  protected:
    // Choices about master selection can hinge on if any decks have sync
    // mode enabled.  This utility method returns true if it finds a deck
    // not in SYNC_NONE mode.
    bool syncDeckExists() const;
    // Choices about master selection can hinge on how many decks are playing
    // back. This utility method counts the number of decks not in SYNC_NONE
    // mode that are playing.
    int playingSyncDeckCount() const;
    // Return the current BPM of the master Syncable. If no master syncable is
    // set then returns the BPM of the internal clock.
    double masterBpm() const;
    // Returns the current beat distance of the master Syncable. If no master
    // Syncable is set, then returns the beat distance of the internal clock.
    double masterBeatDistance() const;
    // Returns the current BPM of the master Syncable if it were playing
    // at 1.0 rate.
    double masterBaseBpm() const;
    // Set the BPM on every sync-enabled Syncable except pSource.
    void setMasterBpm(Syncable* pSource, double bpm);
    // Set the master instantaneous BPM on every sync-enabled Syncable except
    // pSource.
    void setMasterInstantaneousBpm(Syncable* pSource, double bpm);
    // Set the master base bpm, which is what the bpm would be if the syncable
    // were playing at 1.0x speed
    void setMasterBaseBpm(Syncable* pSource, double bpm);
    // Set the master beat distance on every sync-enabled Syncable except
    // pSource.
    void setMasterBeatDistance(Syncable* pSource, double beat_distance);
    void setMasterParams(Syncable* pSource, double beat_distance,double base_bpm, double bpm);
    // Check if there is only one playing syncable deck, and notify it if so.
    void checkUniquePlayingSyncable();
    UserSettingsPointer m_pConfig;
    // The InternalClock syncable.
    InternalClock* m_pInternalClock;
    // The current Syncable that is the master.
    Syncable* m_pMasterSyncable;
    // addSyncableDeck.
    QList<Syncable*> m_syncables;

  private:
    // Activate a specific syncable as master.
    void activateMaster(Syncable* pSyncable);
    // Activate a specific channel as Follower. Sets the syncable's bpm and
    // beat_distance to match the master.
    void activateFollower(Syncable* pSyncable);
    // Unsets all sync state on a Syncable.
    void deactivateSync(Syncable* pSyncable);
};

#endif
