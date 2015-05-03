#ifndef LIBRARYCONTROL_H
#define LIBRARYCONTROL_H

#include <QObject>

#include "control/controlobjectthread.h"

class ControlObject;
class ControlPushButton;
class Library;
class WLibrary;
class WLibrarySidebar;
class MixxxKeyboard;

class LoadToGroupController : public QObject {
    Q_OBJECT
  public:
    LoadToGroupController(QObject* pParent, const QString& group);
    virtual ~LoadToGroupController();

  signals:
    void loadToGroup(QString group, bool);

  public slots:
    void onLoadToGroup(double v);
    void onLoadToGroupAndPlay(double v);

  private:
    QString m_group;
    ControlObject* m_pLoadControl;
    ControlObject* m_pLoadAndPlayControl;
};

class LibraryControl : public QObject {
    Q_OBJECT
  public:
    LibraryControl(Library* pLibrary);
    virtual ~LibraryControl();

    void bindWidget(WLibrary* pLibrary, MixxxKeyboard* pKeyboard);
    void bindSidebarWidget(WLibrarySidebar* pLibrarySidebar);

  private slots:
    void libraryWidgetDeleted();
    void sidebarWidgetDeleted();
    void onLoadSelectedTrackToGroup(QString group, bool play);
    void onSelectNextTrack(double v);
    void onSelectPrevTrack(double v);
    void onSelectTrack(double v);
    void onSelectSidebarItem(double v);
    void onSelectNextSidebarItem(double v);
    void onSelectPrevSidebarItem(double v);
    void onToggleSelectedSidebarItem(double v);
    void onLoadSelectedIntoFirstStopped(double v);
    void onAutoDjAddTop(double v);
    void onAutoDjAddBottom(double v);

    void maybeCreateGroupController(const QString& group);
    void onNumDecksChanged(double v);
    void onNumSamplersChanged(double v);
    void onNumPreviewDecksChanged(double v);

    void onFontSize(double v);
    void onIncrementFontSize(double v);
    void onDecrementFontSize(double v);

  private:
    Library* m_pLibrary;

    ControlObject* m_pSelectNextTrack;
    ControlObject* m_pSelectPrevTrack;
    ControlObject* m_pSelectTrack;

    ControlObject* m_pSelectSidebarItem;
    ControlObject* m_pSelectPrevSidebarItem;
    ControlObject* m_pSelectNextSidebarItem;

    ControlObject* m_pToggleSidebarItem;
    ControlObject* m_pLoadSelectedIntoFirstStopped;
    ControlObject* m_pAutoDjAddTop;
    ControlObject* m_pAutoDjAddBottom;

    ControlObject* m_pFontSizeKnob;
    ControlPushButton* m_pFontSizeIncrement;
    ControlPushButton* m_pFontSizeDecrement;

    WLibrary* m_pLibraryWidget;
    WLibrarySidebar* m_pSidebarWidget;
    ControlObjectThread m_numDecks;
    ControlObjectThread m_numSamplers;
    ControlObjectThread m_numPreviewDecks;
    QMap<QString, LoadToGroupController*> m_loadToGroupControllers;
};

#endif //LIBRARYCONTROL_H
