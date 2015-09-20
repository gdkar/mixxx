#include "controllers/defs_controllers.h"
QString resourcePresetsPath(ConfigObject<ConfigValue>* pConfig) {
    return QDir{pConfig->getResourcePath().append("/controllers/")}.absolutePath().append("/");
}
// Prior to Mixxx 1.11.0 presets were stored in ${SETTINGS_PATH}/midi.
QString legacyUserPresetsPath(ConfigObject<ConfigValue>* pConfig) {
    return QDir{pConfig->getSettingsPath().append("/midi/")}.absolutePath().append("/");
}
QString userPresetsPath(ConfigObject<ConfigValue>* pConfig) {
    return QDir(pConfig->getSettingsPath().append("/controllers/")).absolutePath().append("/");
}
