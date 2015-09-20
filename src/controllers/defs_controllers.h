/***************************************************************************
                          defs_controllers.h
                          ------------------
    copyright            : (C) 2011 by Sean Pappalardo
    email                : spappalardo@mixxx.org
 ***************************************************************************/

#include <QDir>
#include "configobject.h"

QString resourcePresetsPath(ConfigObject<ConfigValue>* pConfig);
// Prior to Mixxx 1.11.0 presets were stored in ${SETTINGS_PATH}/midi.
QString legacyUserPresetsPath(ConfigObject<ConfigValue>* pConfig);
QString userPresetsPath(ConfigObject<ConfigValue>* pConfig);
#define HID_PRESET_EXTENSION ".hid.xml"
#define MIDI_PRESET_EXTENSION ".midi.xml"
#define BULK_PRESET_EXTENSION ".bulk.xml"
#define REQUIRED_SCRIPT_FILE "common-controller-scripts.js"
#define XML_SCHEMA_VERSION "1"
