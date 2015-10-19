/**
* @file controllerpresetinfoenumerator.cpp
* @author Be be.0@gmx.com
* @date Sat Jul 18 2015
* @brief Enumerate list of available controller mapping presets
*/
#include <QDirIterator>

#include "controllers/controllerpresetinfo.h"
#include "controllers/controllerpresetinfoenumerator.h"

#include "controllers/defs_controllers.h"

PresetInfoEnumerator::PresetInfoEnumerator(ConfigObject<ConfigValue>* pConfig)
{
    m_controllerDirPaths.append(userPresetsPath(pConfig));
    m_controllerDirPaths.append(resourcePresetsPath(pConfig));

    // Static list of supported default extensions, sorted by popularity
    m_fileExtensions.append(QString(MIDI_PRESET_EXTENSION));
    m_fileExtensions.append(QString(HID_PRESET_EXTENSION));
    m_fileExtensions.append(QString(BULK_PRESET_EXTENSION));

    loadSupportedPresets();
}
PresetInfoEnumerator::~PresetInfoEnumerator()
{
    for (auto it = m_presetFileHandlersByExtension.begin();
         it != m_presetFileHandlersByExtension.end(); ++it)
    {
        delete it.value();
        it = m_presetFileHandlersByExtension.erase(it);
    }
}
bool PresetInfoEnumerator::isValidExtension(const QString extension)
{
    if (m_presetsByExtension.contains(extension))return true;
    return false;
}

bool PresetInfoEnumerator::hasPresetInfo(const QString extension, const QString name)
{
    // Check if preset info matching extension and preset name can be found
    if (!isValidExtension(extension)) return false;
    for (auto it = m_presetsByExtension.cbegin(); it != m_presetsByExtension.cend(); ++it)
    {
        for (auto it2 = it.value().cbegin(); it2 != it.value().cend(); ++it2) {
            if (name == it2.value().getName()) return true;
        }
    }
    return false;
}
bool PresetInfoEnumerator::hasPresetInfo(const QString path)
{
    for(auto extension: m_presetsByExtension.keys())
    {
        auto presets = m_presetsByExtension[extension];
        if (presets.contains(path)) return true;
    }
    return false;
}

PresetInfo PresetInfoEnumerator::getPresetInfo(const QString path)
{
    // Lookup and return controller script preset info by script path
    // Return NULL if path is not found.
    for(auto extension: m_presetsByExtension.keys())
    {
        auto presets = m_presetsByExtension[extension];
        if (presets.contains(path)) return presets[path];
    }
    return PresetInfo();
}
QList<PresetInfo> PresetInfoEnumerator::getPresets(const QString extension)
{
    // Return list of PresetInfo items matching extension
    // Returns empty list if no matching extension presets can be found
    QList<PresetInfo> presets;
    if (m_presetsByExtension.contains(extension))
    {
        presets = m_presetsByExtension[extension].values();
        return m_presetsByExtension[extension].values();
    }
    qDebug() << "Extension not registered to presetinfo" << extension;
    return presets;
}

void PresetInfoEnumerator::addExtension(const QString extension) {
    if (m_presetsByExtension.contains(extension))
        return;
    QMap<QString,PresetInfo> presets;
    m_presetsByExtension[extension] = presets;
}
void PresetInfoEnumerator::loadSupportedPresets()
{
    for(auto dirPath: m_controllerDirPaths)
    {
        QDirIterator it(dirPath);
        while (it.hasNext())
        {
            it.next();
            auto path = it.filePath();
            for(auto extension: m_fileExtensions)
            {
                if (!path.endsWith(extension)) continue;
                if (!m_presetsByExtension.contains(extension)) addExtension(extension);
                m_presetsByExtension[extension][path] = PresetInfo(path);
            }
        }
    }
    for(auto extension: m_presetsByExtension.keys())
    {
        QMap<QString,PresetInfo> presets = m_presetsByExtension[extension];
        qDebug() << "Extension" << extension << "total" << presets.keys().length() << "presets";
    }
}
void PresetInfoEnumerator::updatePresets(const QString extension)
{
    QMap<QString,PresetInfo> presets;
    if (m_presetsByExtension.contains(extension)) m_presetsByExtension.remove(extension);
    for(auto dirPath: m_controllerDirPaths) {
        QDirIterator it(dirPath);
        while (it.hasNext())
        {
            it.next();
            auto path = it.filePath();
            if (!path.endsWith(extension)) continue;
            presets[path] = PresetInfo(path);
        }
    }
    m_presetsByExtension[extension] = presets;
}
