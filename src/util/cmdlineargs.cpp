#include "util/cmdlineargs.h"
CmdlineArgs& CmdlineArgs::Instance()
{
    static CmdlineArgs cla;
    return cla;
}
bool CmdlineArgs::Parse(int &argc, char **argv)
{
    for (int i = 0; i < argc; ++i) {
        if (   argv[i] == QString("-h")
            || argv[i] == QString("--h")
            || argv[i] == QString("--help")) {
            return false; // Display Help Message
        }

        if (argv[i]==QString("-f").toLower() || argv[i]==QString("--f") || argv[i]==QString("--fullScreen"))
        {
            m_startInFullscreen = true;
        } else if (argv[i] == QString("--locale") && i+1 < argc) {
            m_locale = argv[i+1];
        } else if (argv[i] == QString("--settingsPath") && i+1 < argc) {
            m_settingsPath = QString::fromLocal8Bit(argv[i+1]);
            // TODO(XXX) Trailing slash not needed anymore as we switches from String::append
            // to QDir::filePath elsewhere in the code. This is candidate for removal.
            if (!m_settingsPath.endsWith("/")) {
                m_settingsPath.append("/");
            }
            m_settingsPathSet=true;
        } else if (argv[i] == QString("--resourcePath") && i+1 < argc) {
            m_resourcePath = QString::fromLocal8Bit(argv[i+1]);
            i++;
        } else if (argv[i] == QString("--pluginPath") && i+1 < argc) {
            m_pluginPath = QString::fromLocal8Bit(argv[i+1]);
            i++;
        } else if (argv[i] == QString("--timelinePath") && i+1 < argc) {
            m_timelinePath = QString::fromLocal8Bit(argv[i+1]);
            i++;
        } else if (QString::fromLocal8Bit(argv[i]).contains("--controllerDebug", Qt::CaseInsensitive)) {
            m_controllerDebug = true;
        } else if (QString::fromLocal8Bit(argv[i]).contains("--developer", Qt::CaseInsensitive)) {
            m_developer = true;

        } else if (QString::fromLocal8Bit(argv[i]).contains("--safeMode", Qt::CaseInsensitive)) {
            m_safeMode = true;
        } else {
            m_musicFiles += QString::fromLocal8Bit(argv[i]);
        }
    }
    return true;
}
CmdlineArgs::CmdlineArgs() :
      m_startInFullscreen(false), // Initialize vars
      m_controllerDebug(false),
      m_developer(false),
      m_safeMode(false),
      m_settingsPathSet(false),
// We are not ready to switch to XDG folders under Linux, so keeping $HOME/.mixxx as preferences folder. see lp:1463273
#ifdef __LINUX__
      m_settingsPath(QDir::homePath().append("/").append(SETTINGS_PATH))
{
#else
      // TODO(XXX) Trailing slash not needed anymore as we switches from String::append
      // to QDir::filePath elsewhere in the code. This is candidate for removal.
      m_settingsPath(QDesktopServices::storageLocation(QDesktopServices::DataLocation).append("/"))
  {
#endif
  }
CmdlineArgs::~CmdlineArgs() = default;
QList<QString> CmdlineArgs::getMusicFiles() const
{ 
  return m_musicFiles;
}
bool CmdlineArgs::getStartInFullscreen() const
{ 
  return m_startInFullscreen;
}
bool CmdlineArgs::getControllerDebug() const
{ 
  return m_controllerDebug;
}
bool CmdlineArgs::getDeveloper() const
{ 
  return m_developer;
}
bool CmdlineArgs::getSafeMode() const
{ 
  return m_safeMode;
}
bool CmdlineArgs::getSettingsPathSet() const
{ 
  return m_settingsPathSet;
}
bool CmdlineArgs::getTimelineEnabled() const
{ 
  return !m_timelinePath.isEmpty();
}
QString CmdlineArgs::getLocale() const
{ 
  return m_locale;
}
QString CmdlineArgs::getSettingsPath() const
{ 
  return m_settingsPath; }
void CmdlineArgs::setSettingsPath(QString newSettingsPath)
{ 
  m_settingsPath = newSettingsPath;
}
QString CmdlineArgs::getResourcePath() const
{ 
  return m_resourcePath;
}
QString CmdlineArgs::getPluginPath() const
{
  return m_pluginPath;
}
QString CmdlineArgs::getTimelinePath() const
{ 
  return m_timelinePath;
}
