#include "engine/sync/syncable.h"

Syncable::~Syncable()=default;

SyncMode syncModeFromDouble(double value)
{
    // msvs does not allow to cast from double to an enum
    auto mode = static_cast<SyncMode>(int(value));
    if (mode >= SYNC_NUM_MODES || mode < 0) return SYNC_NONE;
    return mode;
}

