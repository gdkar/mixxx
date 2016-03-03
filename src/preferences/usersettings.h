_Pragma("once")
#include <QSharedPointer>
#include <QWeakPointer>

#include "configobject.h"

typedef ConfigObject<ConfigValue> UserSettings;
typedef QSharedPointer<UserSettings> UserSettingsPointer;
typedef QWeakPointer<UserSettings> UserSettingsWeakPointer;
