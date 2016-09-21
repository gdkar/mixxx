/**
* @file controllerpreset.h
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Mon 9 Apr 2012
* @brief Controller preset
*
* This class represents a controller preset, containing the data elements that
* make it up.
*/

#ifndef CONTROLLERPRESET_H
#define CONTROLLERPRESET_H

#include <QHash>
#include "controllers/controllerpresetvisitor.h"
#include <QSharedPointer>
#include <QString>
#include <QList>

class ControllerPreset {
  public:
    ControllerPreset() = default;
    virtual ~ControllerPreset() = default;
    struct ScriptFileInfo {
        ScriptFileInfo()
                : builtin(false) {

        }
        QString name;
        QString functionPrefix;
        bool builtin;
    };
    /** addScriptFile(QString,QString)
     * Adds an entry to the list of script file names & associated list of function prefixes
     * @param filename Name of the XML file to add
     * @param functionprefix Function prefix to add
     */
    void addScriptFile(QString filename, QString functionprefix, bool builtin=false) {
        ScriptFileInfo info;
        info.name = filename;
        info.functionPrefix = functionprefix;
        info.builtin = builtin;
        scripts.append(info);
    }

    void setDeviceId(const QString id) {
        m_deviceId = id;
    }

    QString deviceId() const {
        return m_deviceId;
    }

    void setFilePath(const QString filePath) {
        m_filePath = filePath;
    }

    QString filePath() const {
        return m_filePath;
    }

    void setName(const QString name) {
        m_name = name;
    }

    QString name() const {
        return m_name;
    }

    void setAuthor(const QString author) {
        m_author = author;
    }

    QString author() const {
        return m_author;
    }

    void setDescription(const QString description) {
        m_description = description;
    }

    QString description() const {
        return m_description;
    }

    void setForumLink(const QString forumlink) {
        m_forumlink = forumlink;
    }

    QString forumlink() const {
        return m_forumlink;
    }

    void setWikiLink(const QString wikilink) {
        m_wikilink = wikilink;
    }

    QString wikilink() const {
        return m_wikilink;
    }

    void setSchemaVersion(const QString schemaVersion) {
        m_schemaVersion = schemaVersion;
    }

    QString schemaVersion() const {
        return m_schemaVersion;
    }

    void setMixxxVersion(const QString mixxxVersion) {
        m_mixxxVersion = mixxxVersion;
    }

    QString mixxxVersion() const {
        return m_mixxxVersion;
    }

    void addProductMatch(QHash<QString,QString> match) {
        m_productMatches.append(match);
    }

    virtual void accept(ControllerPresetVisitor* visitor)  {
        if(visitor) visitor->visit(this);
    }
    virtual void accept(ConstControllerPresetVisitor* visitor) const {
        if(visitor) visitor->visit(this);
    }
    virtual bool isMappable() const {
        return false;
    }

    QList<ScriptFileInfo> scripts;
    // Optional list of controller device match details
    QList< QHash<QString,QString> > m_productMatches;

  private:
    QString m_deviceId;
    QString m_filePath;
    QString m_name;
    QString m_author;
    QString m_description;
    QString m_forumlink;
    QString m_wikilink;
    QString m_schemaVersion;
    QString m_mixxxVersion;
};

typedef QSharedPointer<ControllerPreset> ControllerPresetPointer;

#endif
