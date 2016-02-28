#include "controllerpreset.h"

ControllerPreset::ControllerPreset() = default;
ControllerPreset::~ControllerPreset() = default;

void ControllerPreset::addScriptFile(QString filename, QString functionprefix,bool builtin)
{
    ScriptFileInfo info;
    info.name = filename;
    info.functionPrefix = functionprefix;
    info.builtin = builtin;
    scripts.append(info);
}
void ControllerPreset::setDeviceId(const QString id) {
    m_deviceId = id;
}

QString ControllerPreset::deviceId() const {
    return m_deviceId;
}

void ControllerPreset::setFilePath(const QString filePath) {
    m_filePath = filePath;
}

QString ControllerPreset::filePath() const {
    return m_filePath;
}

void ControllerPreset::setName(const QString name) {
    m_name = name;
}

QString ControllerPreset::name() const {
    return m_name;
}

void ControllerPreset::setAuthor(const QString author) {
    m_author = author;
}

QString ControllerPreset::author() const {
    return m_author;
}

void ControllerPreset::setDescription(const QString description) {
    m_description = description;
}

QString ControllerPreset::description() const {
    return m_description;
}

void ControllerPreset::setForumLink(const QString forumlink) {
    m_forumlink = forumlink;
}

QString ControllerPreset::forumlink() const {
    return m_forumlink;
}

void ControllerPreset::setWikiLink(const QString wikilink) {
    m_wikilink = wikilink;
}

QString ControllerPreset::wikilink() const {
    return m_wikilink;
}

void ControllerPreset::setSchemaVersion(const QString schemaVersion) {
    m_schemaVersion = schemaVersion;
}

QString ControllerPreset::schemaVersion() const {
    return m_schemaVersion;
}

void ControllerPreset::setMixxxVersion(const QString mixxxVersion) {
    m_mixxxVersion = mixxxVersion;
}

QString ControllerPreset::mixxxVersion() const {
    return m_mixxxVersion;
}

void ControllerPreset::addProductMatch(QHash<QString,QString> match) {
    m_productMatches.append(match);
}
