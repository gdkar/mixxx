/***************************************************************************
                          dlgprefcontrols.h  -  description
                             -------------------
    begin                : Sat Jul 5 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
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
#include <QWidget>

#include "preferences/ui_dlgprefcontrolsdlg.h"
#include "configobject.h"
#include "preferences/dlgpreferencepage.h"

class ControlPotmeter;
class SkinLoader;
class PlayerManager;
class MixxxMainWindow;
class ControlObject;

/**
  *@author Tue & Ken Haste Andersen
  */

class DlgPrefControls : public DlgPreferencePage, public Ui::DlgPrefControlsDlg  {
    Q_OBJECT
  public:
    DlgPrefControls(QWidget *parent, MixxxMainWindow *mixxx,
                    SkinLoader* pSkinLoader, PlayerManager* pPlayerManager,
                    ConfigObject<ConfigValue> *pConfig);
    virtual ~DlgPrefControls();

  public slots:
    void slotUpdate();
    void slotApply();
    void slotResetToDefaults();

    void slotSetRateRange(int pos);
    void slotSetRateRangePercent(int rateRangePercent);
    void slotSetRateDir(int pos);
    void slotKeylockMode(int pos);
    void slotSetRateTempLeft(double);
    void slotSetRateTempRight(double);
    void slotSetRatePermLeft(double);
    void slotSetRatePermRight(double);
    void slotSetTooltips(int pos);
    void slotSetSkin(int);
    void slotSetScheme(int);
    void slotUpdateSchemes();
    void slotSetPositionDisplay(int);
    void slotSetPositionDisplay(double);
    void slotSetAllowTrackLoadToPlayingDeck(int);
    void slotSetCueDefault(int);
    void slotSetCueRecall(int);
    void slotSetRateRamp(bool);
    void slotSetRateRampSensitivity(int);
    void slotSetLocale(int);
    void slotSetStartInFullscreen(int index);

    void slotNumDecksChanged(double);
    void slotNumSamplersChanged(double);
    
    void slotUpdateSpeedAutoReset(int);

  private:
    void notifyRebootNecessary();
    bool checkSkinResolution(QString skin);

    ConfigObject<ConfigValue>* m_pConfig;
    ControlObject* m_pControlPositionDisplay;
    ControlObject* m_pNumDecks;
    ControlObject* m_pNumSamplers;
    QList<ControlObject*> m_cueControls;
    QList<ControlObject*> m_rateControls;
    QList<ControlObject*> m_rateDirControls;
    QList<ControlObject*> m_rateRangeControls;
    QList<ControlObject*> m_keylockModeControls;
    MixxxMainWindow *m_mixxx;
    SkinLoader* m_pSkinLoader;
    PlayerManager* m_pPlayerManager;

    int m_iNumConfiguredDecks;
    int m_iNumConfiguredSamplers;
    
    int m_speedAutoReset;
    int m_keylockMode;
};
