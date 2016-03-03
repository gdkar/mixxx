#include "controllers/hid/hidcontrollerpresetfilehandler.h"
bool HidControllerPresetFileHandler::save(const HidControllerPreset& preset,
                                          const QString deviceName,
                                          const QString fileName) const {
    auto doc = buildRootWithScripts(preset, deviceName);
    return writeDocument(doc, fileName);
}
ControllerPresetPointer HidControllerPresetFileHandler::load(const QDomElement root,
                                                             const QString deviceName) {
    if (root.isNull())
        return ControllerPresetPointer();
    auto controller = getControllerNode(root, deviceName);
    if (controller.isNull())
        return ControllerPresetPointer();
    auto preset = new HidControllerPreset();
    parsePresetInfo(root, preset);
    addScriptFilesToPreset(controller, preset);
    return ControllerPresetPointer(preset);
}
