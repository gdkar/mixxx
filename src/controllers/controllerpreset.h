/**
* @file controllerpreset.h
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Mon 9 Apr 2012
* @brief Controller preset
*
* This class represents a controller preset, containing the data elements that
* make it up.
*/

_Pragma("once")
#include <QHash>
#include <QSharedPointer>
#include <QString>
#include <QList>

class ControllerPresetVisitor;
class ConstControllerPresetVisitor;

class ControllerPreset {
  public:
    ControllerPreset();
    virtual ~ControllerPreset();
    struct ScriptFileInfo {
        QString name;
        QString functionPrefix;
        bool builtin = false;
    };

    /** addScriptFile(QString,QString)
     * Adds an entry to the list of script file names & associated list of function prefixes
     * @param filename Name of the XML file to add
     * @param functionprefix Function prefix to add
     */
    void addScriptFile(QString filename, QString functionprefix,
                       bool builtin=false);
    void setDeviceId(const QString id);
    QString deviceId() const;
    void setFilePath(const QString filePath);
    QString filePath() const ;
    void setName(const QString name);
    QString name() const;
    void setAuthor(const QString author) ;
    QString author() const;
    void setDescription(const QString description);
    QString description() const;
    void setForumLink(const QString forumlink);
    QString forumlink() const;
    void setWikiLink(const QString wikilink);
    QString wikilink() const;
    void setSchemaVersion(const QString schemaVersion);
    QString schemaVersion() const;
    void setMixxxVersion(const QString mixxxVersion);
    QString mixxxVersion() const;
    void addProductMatch(QHash<QString,QString> match);
    virtual void accept(ControllerPresetVisitor* visitor) = 0;
    virtual void accept(ConstControllerPresetVisitor* visitor) const = 0;
    virtual bool isMappable() const = 0;
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
