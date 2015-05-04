/***************************************************************************
                          mixxx.h  -  description
                             -------------------
    begin                : Mon Feb 18 09:48:17 CET 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MIXXX_H
#define MIXXX_H

#include <QAction>
#include <QList>
#include <QMainWindow>
#include <QString>
#include <QDir>
#include <QQmlEngine>
// REMOVE ME
#include <QtDebug>
#include <QResizeEvent>

class EngineMaster;
class Library;
class LibraryScanner;
class ControllerManager;
class MixxxKeyboard;
class PlayerManager;
class RecordingManager;
class ShoutcastManager;
class SkinLoader;
class EffectsManager;
class VinylControlManager;
class GuiTick;
class DlgPreferences;
class SoundManager;
class ControlPushButton;
class DlgDeveloperTools;

#include "configobject.h"
#include "track/trackinfoobject.h"
#include "util/cmdlineargs.h"
#include "util/timer.h"

class ControlObjectSlave;
class ControlObject;
class QTranslator;
class QQmlComponent;
class QQmlContext;
class QJSEngine;
class QJSValue;

// This Class is the base class for Mixxx. It sets up the main
// window and providing a menubar.
// For the main view, an instance of class MixxxView is
// created which creates your view.
class MixxxMainWindow : public QMainWindow {
    Q_OBJECT

  public:
    // Construtor. files is a list of command line arguments
    MixxxMainWindow(QApplication *app, const CmdlineArgs& args);
    virtual ~MixxxMainWindow();
    // initializes all QActions of the application
    void initActions();
    // initMenuBar creates the menu_bar and inserts the menuitems
    void initMenuBar();

    void setToolTipsCfg(int tt);
    inline int getToolTipsCgf() { return m_toolTipsCfg; }
    void rebootMixxxView();

    inline GuiTick* getGuiTick() { return m_pGuiTick; };

  public slots:

    //void onQuitFullScreen();
    void onFileLoadSongPlayer(int deck);
    // Opens a file in player 1
    void onFileLoadSongPlayer1();
    // Opens a file in player 2
    void onFileLoadSongPlayer2();
    // exits the application
    void onFileQuit();

    // toggle vinyl control - Don't #ifdef this because MOC is dumb
    void onControlVinylControl(int);
    void onCheckboxVinylControl(int);
    void onControlPassthrough(int);
    void onControlAuxiliary(int);
    // toogle keyboard on-off
    void onOptionsKeyboard(bool toggle);
    // Preference dialog
    void onOptionsPreferences();
    // shows an about dlg
    void onHelpAbout();
    // visits support section of website
    void onHelpSupport();
    // Visits a feedback form
    void onHelpFeedback();
    // Open the manual.
    void onHelpManual();
    // Visits translation interface on launchpad.net
    void onHelpTranslation();
    // Scan or rescan the music library directory
    void onScanLibrary();
    // Enables the "Rescan Library" menu item. This gets disabled when a scan is running.
    void onEnableRescanLibraryAction();
    //Updates the checkboxes for Recording and Livebroadcasting when connection drops, or lame is not available
    void onOptionsMenuShow();
    // toogle on-screen widget visibility
    void onViewShowSamplers(bool);
    void onViewShowVinylControl(bool);
    void onViewShowMicrophone(bool);
    void onViewShowPreviewDeck(bool);
    void onViewShowEffects(bool);
    void onViewShowCoverArt(bool);
    void onViewMaximizeLibrary(bool);
    // toogle full screen mode
    void onViewFullScreen(bool toggle);
    // Reload the skin.
    void onDeveloperReloadSkin(bool toggle);
    // Open the developer tools dialog.
    void onDeveloperTools();
    void onDeveloperToolsClosed();
    void onDeveloperStatsExperiment();
    void onDeveloperStatsBase();
    // toogle the script debugger
    void onDeveloperDebugger(bool toggle);

    void onToCenterOfPrimaryScreen();

    void onNewSkinLoaded();

    // Activated when the number of decks changed, so we can update the UI.
    void onNumDecksChanged(double);

    // Activated when the talkover button is pushed on a microphone so we
    // can alert the user if a mic is not configured.
    void onTalkoverChanged(int);

    void onUpdateWindowTitle(TrackPointer pTrack);

    void onToggleCheckedVinylControl();
    void onToggleCheckedSamplers();
    void onToggleCheckedMicrophone();
    void onToggleCheckedPreviewDeck();
    void onToggleCheckedEffects();
    void onToggleCheckedCoverArt();

  signals:
    void newSkinLoaded();
    void libraryScanStarted();
    void libraryScanFinished();
    // used to uncheck the menu when the dialog of develeoper tools is closed
    void developerToolsDlgClosed(int r);
    void closeDeveloperToolsDlgChecked(int r);

  protected:
    // Event filter to block certain events (eg. tooltips if tooltips are disabled)
    virtual bool eventFilter(QObject *obj, QEvent *event);
    virtual void closeEvent(QCloseEvent *event);
    virtual bool event(QEvent* e);

  private:
    void logBuildDetails();
    void initializeWindow();
    void initializeKeyboard();
    void initializeTranslations(QApplication* pApp);
    void initializeFonts();
    bool loadTranslations(const QLocale& systemLocale, QString userLocale,
                          const QString& translation, const QString& prefix,
                          const QString& translationPath, QTranslator* pTranslator);
    void checkDirectRendering();
    bool confirmExit();
    void linkSkinWidget(ControlObjectSlave** pCOS,
                        ConfigKey key, const char* slot);
    void updateCheckedMenuAction(QAction* menuAction, ConfigKey key);

    QQmlEngine      m_qmlEngine;
    QQmlContext    *m_qmlContext;

    // Pointer to the root GUI widget
    QWidget* m_pWidgetParent;

    // The effects processing system
    EffectsManager* m_pEffectsManager;

    // The mixing engine.
    EngineMaster* m_pEngine;

    // The skin loader
    SkinLoader* m_pSkinLoader;

    // The sound manager
    SoundManager *m_pSoundManager;

    // Keeps track of players
    PlayerManager* m_pPlayerManager;
    // RecordingManager
    RecordingManager* m_pRecordingManager;
#ifdef __SHOUTCAST__
    ShoutcastManager* m_pShoutcastManager;
#endif
    ControllerManager* m_pControllerManager;

    ConfigObject<ConfigValue>* m_pConfig;

    GuiTick* m_pGuiTick;

    VinylControlManager* m_pVCManager;

    MixxxKeyboard* m_pKeyboard;
    // Library scanner object
    LibraryScanner* m_pLibraryScanner;
    // The library management object
    Library* m_pLibrary;

    // file_menu contains all items of the menubar entry "File"
    QMenu* m_pFileMenu;
    // edit_menu contains all items of the menubar entry "Edit"
    QMenu* m_pEditMenu;
    // library menu
    QMenu* m_pLibraryMenu;
    // options_menu contains all items of the menubar entry "Options"
    QMenu* m_pOptionsMenu;
    // view_menu contains all items of the menubar entry "View"
    QMenu* m_pViewMenu;
    // view_menu contains all items of the menubar entry "Help"
    QMenu* m_pHelpMenu;
    // Developer options.
    QMenu* m_pDeveloperMenu;

    QAction* m_pFileLoadSongPlayer1;
    QAction* m_pFileLoadSongPlayer2;
    QAction* m_pFileQuit;
    QAction* m_pPlaylistsNew;
    QAction* m_pCratesNew;
    QAction* m_pLibraryRescan;
#ifdef __VINYLCONTROL__
    QMenu* m_pVinylControlMenu;
    QList<QAction*> m_pOptionsVinylControl;
#endif
    QAction* m_pOptionsRecord;
    QAction* m_pOptionsKeyboard;

    QAction* m_pOptionsPreferences;
#ifdef __SHOUTCAST__
    QAction* m_pOptionsShoutcast;
#endif
    QAction* m_pViewShowSamplers;
    QAction* m_pViewVinylControl;
    QAction* m_pViewShowMicrophone;
    QAction* m_pViewShowPreviewDeck;
    QAction* m_pViewShowEffects;
    QAction* m_pViewShowCoverArt;
    QAction* m_pViewMaximizeLibrary;
    QAction* m_pViewFullScreen;
    QAction* m_pHelpAboutApp;
    QAction* m_pHelpSupport;
    QAction* m_pHelpFeedback;
    QAction* m_pHelpTranslation;
    QAction* m_pHelpManual;

    QAction* m_pDeveloperReloadSkin;
    QAction* m_pDeveloperTools;
    QAction* m_pDeveloperStatsExperiment;
    QAction* m_pDeveloperStatsBase;
    DlgDeveloperTools* m_pDeveloperToolsDlg;
    QAction* m_pDeveloperDebugger;

    ControlObjectSlave* m_pShowVinylControl;
    ControlObjectSlave* m_pShowSamplers;
    ControlObjectSlave* m_pShowMicrophone;
    ControlObjectSlave* m_pShowPreviewDeck;
    ControlObjectSlave* m_pShowEffects;
    ControlObjectSlave* m_pShowCoverArt;
    ControlObject* m_pNumAuxiliaries;

    int m_iNoPlaylists;

    /** Pointer to preference dialog */
    DlgPreferences* m_pPrefDlg;

    int noSoundDlg(void);
    int noOutputDlg(bool* continueClicked);
    // Fullscreen patch
    QPoint m_winpos;
    bool m_NativeMenuBarSupport;

    ConfigObject<ConfigValueKbd>* m_pKbdConfig;
    ConfigObject<ConfigValueKbd>* m_pKbdConfigEmpty;

    int m_toolTipsCfg; //0=OFF, 1=ON, 2=ON (only in Library)
    // Timer that tracks how long Mixxx has been running.
    Timer m_runtime_timer;

    const CmdlineArgs& m_cmdLineArgs;

    ControlPushButton* m_pTouchShift;
    QList<ControlObjectSlave*> m_pVinylControlEnabled;
    QList<ControlObjectSlave*> m_pPassthroughEnabled;
    QList<ControlObjectSlave*> m_pAuxiliaryPassthrough;
    ControlObjectSlave* m_pNumDecks;
    int m_iNumConfiguredDecks;
    QList<ControlObjectSlave*> m_micTalkoverControls;
    QSignalMapper* m_VCControlMapper;
    QSignalMapper* m_VCCheckboxMapper;
    QSignalMapper* m_PassthroughMapper;
    QSignalMapper* m_AuxiliaryMapper;
    QSignalMapper* m_TalkoverMapper;

    static const int kMicrophoneCount;
    static const int kAuxiliaryCount;
};

#endif
