#include "engine/sync/syncable.h"

Syncable::Syncable()=default;
Syncable::~Syncable()=default;
QString Syncable::getGroup()const{return QString{};}
EngineChannel*Syncable::getChannel() const{return nullptr;}
void Syncable::notifySyncModeChanged(SyncMode){}
void Syncable::notifyOnlyPlayingSyncable(){}
void Syncable::requestSyncPhase(){}
SyncMode Syncable::getSyncMode()const{return SYNC_INVALID;}
bool Syncable::isPlaying()const{return false;}
double Syncable::getBpm()const{return 0;}
double Syncable::getBeatDistance()const{return 0;}
double Syncable::getBaseBpm()const{return 0;}
void   Syncable::setMasterBpm(double){}
void   Syncable::setMasterBeatDistance(double){}
void   Syncable::setMasterBaseBpm(double){}
void   Syncable::setMasterParams(double,double,double){}
void   Syncable::setInstantaneousBpm(double){}

SyncableListener::SyncableListener(QObject*pParent):QObject(pParent){}
SyncableListener::~SyncableListener()=default;
void SyncableListener::requestSyncMode(Syncable*,SyncMode){}
void SyncableListener::requestEnableSync(Syncable*,bool){}
void SyncableListener::notifyBpmChanged(Syncable*,double,bool){}
void SyncableListener::notifyInstantaneousBpmChanged(Syncable*,double){}
void SyncableListener::notifyScratching(Syncable*,bool){}
void SyncableListener::notifyBeatDistanceChanged(Syncable*,double){}
void SyncableListener::notifyPlaying(Syncable*,bool){}
void SyncableListener::notifyTrackLoaded(Syncable*,double){}

