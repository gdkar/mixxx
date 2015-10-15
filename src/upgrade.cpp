/***************************************************************************
                          upgrade.cpp  -  description
                             -------------------
    begin                : Fri Mar 13 2009
    copyright            : (C) 2009 by Sean M. Pappalardo
    email                : pegasus@c64.org
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QPixmap>
#include <QMessageBox>
#include <QPushButton>
#include <QTranslator>
#include <QScopedPointer>

#include "defs_version.h"
#include "controllers/defs_controllers.h"
#include "track/beat_preferences.h"
#include "library/trackcollection.h"
#include "library/library_preferences.h"
#include "util/math.h"
#include "util/cmdlineargs.h"
#include "configobject.h"
#include "upgrade.h"


Upgrade::Upgrade(QObject*pParent):QObject(pParent) {};
Upgrade::~Upgrade() = default;
// static 
QString Upgrade::mixxx17HomePath() {
#ifdef __LINUX__
    return QDir::homePath().append("/").append(".mixxx/");
#elif __WINDOWS__
    return QDir::homePath().append("/").append("Local Settings/Application Data/Mixxx/");
#elif __APPLE__
    return QDir::homePath().append("/").append(".mixxx/");
#endif
}
// We return the ConfigObject here because we have to make changes to the
// configuration and the location of the file may change between releases.
ConfigObject<ConfigValue>* Upgrade::versionUpgrade(const QString& settingsPath) {
/*  Pre-1.7.0:
*
*   Since we didn't store version numbers in the config file prior to 1.7.0,
*   we check to see if the user is upgrading if his config files are in the old location,
*   since we moved them in 1.7.0. This code takes care of moving them.
*/
    QDir oldLocation = QDir(QDir::homePath());
/***************************************************************************
*                           Post-1.7.0 upgrade code
*
*   Add entries to the IF ladder below if anything needs to change from the
*   previous to the current version. This allows for incremental upgrades
*   incase a user upgrades from a few versions prior.
****************************************************************************/
    // Read the config file from home directory
    auto config = new ConfigObject<ConfigValue>(QDir(settingsPath).filePath(SETTINGS_FILE));
    auto configVersion = config->getValueString(ConfigKey("Config","Version"));
    if (configVersion.isEmpty()) {
#ifdef __APPLE__
        qDebug() << "Config version is empty, trying to read pre-1.9.0 config";
        // Try to read the config from the pre-1.9.0 final directory on OS X (we moved it in 1.9.0 final)
        QScopedPointer<QFile> oldConfigFile(new QFile(QDir::homePath().append("/").append(".mixxx/mixxx.cfg")));
        if (oldConfigFile->exists() && ! CmdlineArgs::Instance().getSettingsPathSet()) {
            qDebug() << "Found pre-1.9.0 config for OS X";
            // Note: We changed SETTINGS_PATH in 1.9.0 final on OS X so it must be hardcoded to ".mixxx" here for legacy.
            config = new ConfigObject<ConfigValue>(QDir::homePath().append("/.mixxx/mixxx.cfg"));
            // Just to be sure all files like logs and soundconfig go with mixxx.cfg
            // TODO(XXX) Trailing slash not needed anymore as we switches from String::append
            // to QDir::filePath elsewhere in the code. This is candidate for removal.
            CmdlineArgs::Instance().setSettingsPath(QDir::homePath().append("/.mixxx/"));
            configVersion = config->getValueString(ConfigKey("Config","Version"));
        }
        else {
#elif __WINDOWS__
        qDebug() << "Config version is empty, trying to read pre-1.12.0 config";
        // Try to read the config from the pre-1.12.0 final directory on Windows (we moved it in 1.12.0 final)
        QScopedPointer<QFile> oldConfigFile(new QFile(QDir::homePath().append("/Local Settings/Application Data/Mixxx/mixxx.cfg")));
        if (oldConfigFile->exists() && ! CmdlineArgs::Instance().getSettingsPathSet()) {
            qDebug() << "Found pre-1.12.0 config for Windows";
            // Note: We changed SETTINGS_PATH in 1.12.0 final on Windows so it must be hardcoded to "Local Settings/Application Data/Mixxx/" here for legacy.
            config = new ConfigObject<ConfigValue>(QDir::homePath().append("/Local Settings/Application Data/Mixxx/mixxx.cfg"));
            // Just to be sure all files like logs and soundconfig go with mixxx.cfg
            // TODO(XXX) Trailing slash not needed anymore as we switches from String::append
            // to QDir::filePath elsewhere in the code. This is candidate for removal.
            CmdlineArgs::Instance().setSettingsPath(QDir::homePath().append("/Local Settings/Application Data/Mixxx/")); 
            configVersion = config->getValueString(ConfigKey("Config","Version"));
        }
        else {
#endif
            // This must have been the first run... right? :)
            qDebug() << "No version number in configuration file. Setting to" << VERSION;
            config->set(ConfigKey("Config","Version"), ConfigValue(VERSION));
            m_bFirstRun = true;
            return config;
#ifdef __APPLE__
        }
#elif __WINDOWS__
        }
#endif
    }
    // If it's already current, stop here
    if (configVersion == VERSION) {
        qDebug() << "Configuration file is at the current version" << VERSION;
        return config;
    }
    // We use the following blocks to detect if this is the first time
    // you've run the latest version of Mixxx. This lets us show
    // the promo tracks stats agreement stuff for all users that are
    // upgrading Mixxx.

    if (configVersion.startsWith("1.11")) {
        qDebug() << "Upgrading from v1.11.x...";
        // upgrade to the multi library folder settings
        auto currentFolder = config->getValueString(PREF_LEGACY_LIBRARY_DIR);
        // to migrate the DB just add the current directory to the new
        // directories table
        TrackCollection tc(config);
        auto directoryDAO = tc.getDirectoryDAO();
        // NOTE(rryan): We don't have to ask for sandbox permission to this
        // directory because the normal startup integrity check in Library will
        // notice if we don't have permission and ask for access. Also, the
        // Sandbox isn't setup yet at this point in startup because it relies on
        // the config settings path and this function is what loads the config
        // so it's not ready yet.
        auto successful = directoryDAO.addDirectory(currentFolder);
        // ask for library rescan to activate cover art. We can later ask for
        // this variable when the library scanner is constructed.
        m_bRescanLibrary = askReScanLibrary();
        // Versions of mixxx until 1.11 had a hack that multiplied gain by 1/2,
        // which was compensation for another hack that set replaygain to a
        // default of 6.  We've now removed all of the hacks, so subtracting
        // 6 from everyone's replay gain should keep things consistent for
        // all users.
        auto oldReplayGain = config->getValueString(ConfigKey("ReplayGain", "InitialReplayGainBoost"), "6").toInt();
        auto newReplayGain = math_max(-6, oldReplayGain - 6);
        config->set(ConfigKey("ReplayGain", "InitialReplayGainBoost"),ConfigValue(newReplayGain));
        // if everything until here worked fine we can mark the configuration as
        // updated
        if (successful) {
            configVersion = VERSION;
            m_bUpgraded = true;
            config->set(ConfigKey("Config","Version"), ConfigValue(VERSION));
        }
        else {qDebug() << "Upgrade failed!\n";}
    }
    if (configVersion == VERSION) qDebug() << "Configuration file is now at the current version" << VERSION;
    else {
        /* Way too verbose, this confuses the hell out of Linux users when they see this:
        qWarning() << "Configuration file is at version" << configVersion
                   << "and I don't know how to upgrade it to the current" << VERSION
                   << "\n   (That means a function to do this needs to be added to upgrade.cpp.)"
                   << "\n-> Leaving the configuration file version as-is.";
        */
        qWarning() << "Configuration file is at version" << configVersion << "instead of the current" << VERSION;
    }
    return config;
}
bool Upgrade::askReScanLibrary() {
    QMessageBox msgBox;
    msgBox.setIconPixmap(QPixmap(":/images/mixxx-icon.png"));
    msgBox.setWindowTitle(QMessageBox::tr("Upgrading Mixxx"));
    msgBox.setText(QMessageBox::tr("Mixxx now supports displaying cover art.\n"
                      "Do you want to scan your library for cover files now?"));
    auto rescanButton = msgBox.addButton(QMessageBox::tr("Scan"), QMessageBox::AcceptRole);
    msgBox.addButton(QMessageBox::tr("Later"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(rescanButton);
    msgBox.exec();
    return msgBox.clickedButton() == rescanButton;
}

bool Upgrade::askReanalyzeBeats() {
    auto windowTitle =
            QMessageBox::tr("Upgrading Mixxx from v1.9.x/1.10.x.");
    auto mainHeading =
            QMessageBox::tr("Mixxx has a new and improved beat detector.");
    auto paragraph1 = QMessageBox::tr(
        "When you load tracks, Mixxx can re-analyze them "
        "and generate new, more accurate beatgrids. This will make "
        "automatic beatsync and looping more reliable.");
    auto paragraph2 = QMessageBox::tr(
        "This does not affect saved cues, hotcues, playlists, or crates.");
    auto paragraph3 = QMessageBox::tr(
        "If you do not want Mixxx to re-analyze your tracks, choose "
        "\"Keep Current Beatgrids\". You can change this setting at any time "
        "from the \"Beat Detection\" section of the Preferences.");
    auto keepCurrent = QMessageBox::tr("Keep Current Beatgrids");
    auto generateNew = QMessageBox::tr("Generate New Beatgrids");
    QMessageBox msgBox;
    msgBox.setIconPixmap(QPixmap(":/images/mixxx-icon.png"));
    msgBox.setWindowTitle(windowTitle);
    msgBox.setText(QString("<html><h2>%1</h2><p>%2</p><p>%3</p><p>%4</p></html>")
                   .arg(mainHeading, paragraph1, paragraph2, paragraph3));
    msgBox.addButton(keepCurrent, QMessageBox::NoRole);
    auto OverwriteButton = msgBox.addButton(generateNew, QMessageBox::YesRole);
    msgBox.setDefaultButton(OverwriteButton);
    msgBox.exec();
    if (msgBox.clickedButton() == (QAbstractButton*)OverwriteButton) {return true;}
    return false;
}

bool Upgrade::isFirstRun()    const { return m_bFirstRun; };
bool Upgrade::isUpgraded()    const { return m_bUpgraded; };
bool Upgrade::rescanLibrary() const {return m_bRescanLibrary; };

