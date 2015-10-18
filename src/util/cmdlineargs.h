_Pragma("once")
#include <QList>
#include <QString>
#include <QDir>
#include <QDesktopServices>

// A structure to store the parsed command-line arguments
class CmdlineArgs {
  public:
    static CmdlineArgs& Instance();
    bool Parse(int &argc, char **argv);
    QList<QString> getMusicFiles() const;
    bool getStartInFullscreen() const;
    bool getControllerDebug() const;
    bool getDeveloper() const;
    bool getSafeMode() const;
    bool getSettingsPathSet() const;
    bool getTimelineEnabled() const;
    QString getLocale() const;
    QString getSettingsPath() const;
    void setSettingsPath(QString newSettingsPath);
    QString getResourcePath() const;
    QString getPluginPath() const;
    QString getTimelinePath() const;
  private:
    CmdlineArgs();
    virtual ~CmdlineArgs();
    QList<QString> m_musicFiles;    // List of files to load into players at startup
    bool m_startInFullscreen  = false;       // Start in fullscreen mode
    bool m_controllerDebug    = false;
    bool m_developer          = false; // Developer Mode
    bool m_safeMode           = false;
    bool m_settingsPathSet    = false; // has --settingsPath been set on command line ?
    QString m_locale;
    QString m_settingsPath;
    QString m_resourcePath;
    QString m_pluginPath;
    QString m_timelinePath;
};
