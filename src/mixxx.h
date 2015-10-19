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

_Pragma("once")
#include <QAction>
#include <QList>
#include <QMainWindow>
#include <QString>
#include <QDir>

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
class DlgPreferences;
class SoundManager;
class ControlPushButton;
class DlgDeveloperTools;
class Upgrade;
class LaunchImage;
class QSignalMapper;
#include "configobject.h"
#include "trackinfoobject.h"
#include "util/cmdlineargs.h"
#include "util/timer.h"

class ControlObject;
class ControlObject;
class QTranslator;

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
    void initalize(QApplication *app, const CmdlineArgs& args);
    void finalize();
    // initializes all QActions of the application
    void initActions();
    // creates the menu_bar and inserts the file Menu
    void initMenuBar();
    // creates the menu_bar and inserts the file Menu
    // after it was inited
    void populateMenuBar();
    void setToolTipsCfg(int tt);
    int getToolTipsCfg() const;
    void rebootMixxxView();
    // progresses the launch image progress bar
    // this must be called from the GUi thread only
    void launchProgress(int progress);
  public slots:
    //void slotQuitFullScreen();
    void slotFileLoadSongPlayer(int deck);
    // Opens a file in player 1
    void slotFileLoadSongPlayer1();
    // Opens a file in player 2
    void slotFileLoadSongPlayer2();
    // exits the application
    void slotFileQuit();

    // toggle vinyl control - Don't #ifdef this because MOC is dumb
    void slotControlVinylControl(int);
    void slotCheckboxVinylControl(int);
    void slotControlPassthrough(int);
    void slotControlAuxiliary(int);
    // toogle keyboard on-off
    void slotOptionsKeyboard(bool toggle);
    // Preference dialog
    void slotOptionsPreferences();
    // Scan or rescan the music library directory
    void slotScanLibrary();
    // Enables the "Rescan Library" menu item. This gets disabled when a scan is running.
    void slotEnableRescanLibraryAction();
    //Updates the checkboxes for Recording and Livebroadcasting when connection drops, or lame is not available
    void slotOptionsMenuShow();
    // toogle on-screen widget visibility
    void slotViewShowVinylControl(bool);
    void slotViewShowMicrophone(bool);
    void slotViewShowPreviewDeck(bool);
    void slotViewShowEffects(bool);
    // toogle full screen mode
    void slotViewFullScreen(bool toggle);
    // Reload the skin.
    void slotDeveloperReloadSkin(bool toggle);
    // Open the developer tools dialog.
    void slotDeveloperTools();
    void slotDeveloperToolsClosed();
    void slotDeveloperStatsExperiment();
    void slotDeveloperStatsBase();
    // toogle the script debugger
    void slotDeveloperDebugger(bool toggle);
    void slotToCenterOfPrimaryScreen();
    void onNewSkinLoaded();
    // Activated when the number of decks changed, so we can update the UI.
    void slotNumDecksChanged(double);
    // Activated when the talkover button is pushed on a microphone so we
    // can alert the user if a mic is not configured.
    void slotTalkoverChanged(int);
    void slotUpdateWindowTitle(TrackPointer pTrack);
    void slotToggleCheckedVinylControl();
    void slotToggleCheckedSamplers();
    void slotToggleCheckedMicrophone();
    void slotToggleCheckedPreviewDeck();
    void slotToggleCheckedEffects();
    void slotToggleCheckedCoverArt();
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
    bool loadTranslations(QLocale systemLocale, QString userLocale,
                          QString translation, QString prefix,
                          QString translationPath, QTranslator* pTranslator);
    bool confirmExit();
    void linkSkinWidget(ControlObject** pCOS,
                        ConfigKey key, const char* slot);
    void updateCheckedMenuAction(QAction* menuAction, ConfigKey key);

    // Pointer to the root GUI widget
    QWidget* m_pWidgetParent;
    LaunchImage* m_pLaunchImage;

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
    ControllerManager* m_pControllerManager = nullptr;
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    VinylControlManager* m_pVCManager = nullptr;
    MixxxKeyboard* m_pKeyboard = nullptr;
    // Library scanner object
    LibraryScanner* m_pLibraryScanner = nullptr;
    // The library management object
    Library* m_pLibrary = nullptr;
    Upgrade* m_pUpgrader = nullptr;
    // file_menu contains all items of the menubar entry "File"
    QMenu* m_pFileMenu = nullptr;
    // edit_menu contains all items of the menubar entry "Edit"
    QMenu* m_pEditMenu = nullptr;
    // library menu
    QMenu* m_pLibraryMenu = nullptr;
    // options_menu contains all items of the menubar entry "Options"
    QMenu* m_pOptionsMenu = nullptr;
    // view_menu contains all items of the menubar entry "View"
    QMenu* m_pViewMenu = nullptr;
    // view_menu contains all items of the menubar entry "Help"
    QMenu* m_pHelpMenu = nullptr;
    // Developer options.
    QMenu* m_pDeveloperMenu = nullptr;
    QAction* m_pFileLoadSongPlayer1 = nullptr;
    QAction* m_pFileLoadSongPlayer2 = nullptr;
    QAction* m_pFileQuit            = nullptr;
    QAction* m_pPlaylistsNew        = nullptr;
    QAction* m_pCratesNew           = nullptr;
    QAction* m_pLibraryRescan       = nullptr;
#ifdef __VINYLCONTROL__
    QMenu* m_pVinylControlMenu;
    QList<QAction*> m_pOptionsVinylControl;
#endif
    QAction* m_pOptionsRecord       = nullptr;
    QAction* m_pOptionsKeyboard     = nullptr;
    QAction* m_pOptionsPreferences  = nullptr;
#ifdef __SHOUTCAST__
    QAction* m_pOptionsShoutcast    = nullptr;
#endif
    QAction* m_pViewShowSamplers    = nullptr;
    QAction* m_pViewVinylControl    = nullptr;
    QAction* m_pViewShowMicrophone  = nullptr;
    QAction* m_pViewShowPreviewDeck = nullptr;
    QAction* m_pViewShowEffects     = nullptr;
    QAction* m_pViewShowCoverArt    = nullptr;
    QAction* m_pViewMaximizeLibrary = nullptr;
    QAction* m_pViewFullScreen      = nullptr;
    QAction* m_pHelpAboutApp        = nullptr;
    QAction* m_pHelpSupport         = nullptr;
    QAction* m_pHelpFeedback        = nullptr;
    QAction* m_pHelpManual          = nullptr;
    QAction* m_pHelpShortcuts       = nullptr;
    QAction* m_pHelpTranslation     = nullptr;

    QAction* m_pDeveloperReloadSkin = nullptr;
    QAction* m_pDeveloperTools      = nullptr;
    QAction* m_pDeveloperStatsExperiment = nullptr;
    QAction* m_pDeveloperStatsBase  = nullptr;
    DlgDeveloperTools* m_pDeveloperToolsDlg = nullptr;
    QAction* m_pDeveloperDebugger   = nullptr;
    ControlObject* m_pShowVinylControl = nullptr;
    ControlObject* m_pShowSamplers     = nullptr;
    ControlObject* m_pShowMicrophone   = nullptr;
    ControlObject* m_pShowPreviewDeck  = nullptr;
    ControlObject* m_pShowEffects      = nullptr;
    ControlObject* m_pShowCoverArt     = nullptr;
    ControlObject* m_pNumAuxiliaries        = nullptr;

    int m_iNoPlaylists = 0;

    /** Pointer to preference dialog */
    DlgPreferences* m_pPrefDlg = nullptr;
    int noSoundDlg(void);
    int noOutputDlg(bool* continueClicked);
    // Fullscreen patch
    QPoint m_winpos;
    bool m_NativeMenuBarSupport = true;
    ConfigObject<ConfigValueKbd>* m_pKbdConfig = nullptr;
    ConfigObject<ConfigValueKbd>* m_pKbdConfigEmpty = nullptr;
    int m_toolTipsCfg; //0=OFF, 1=ON, 2=ON (only in Library)
    // Timer that tracks how long Mixxx has been running.
    Timer m_runtime_timer;
    const CmdlineArgs& m_cmdLineArgs;
    ControlObject* m_pTouchShift = nullptr;
    QList<ControlObject*> m_pVinylControlEnabled;
    QList<ControlObject*> m_pPassthroughEnabled;
    QList<ControlObject*> m_pAuxiliaryPassthrough;
    ControlObject* m_pNumDecks;
    int m_iNumConfiguredDecks = 0;
    QList<ControlObject*> m_micTalkoverControls;
    QSignalMapper* m_VCControlMapper   = nullptr;
    QSignalMapper* m_VCCheckboxMapper  = nullptr;
    QSignalMapper* m_PassthroughMapper = nullptr;
    QSignalMapper* m_AuxiliaryMapper   = nullptr;
    QSignalMapper* m_TalkoverMapper    = nullptr;
    int kMicrophoneCount = 2;
    int kAuxiliaryCount  = 2;
};
