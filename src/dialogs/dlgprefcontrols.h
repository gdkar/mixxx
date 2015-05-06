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

#ifndef DLGPREFCONTROLS_H
#define DLGPREFCONTROLS_H

#include <QWidget>

#include "dialogs/ui_dlgprefcontrolsdlg.h"
#include "configobject.h"
#include "preferences/dlgpreferencepage.h"

class ControlObjectSlave;
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
    void onUpdate();
    void onApply();
    void onResetToDefaults();

    void onSetRateRange(int pos);
    void onSetRateDir(int pos);
    void onKeylockMode(int pos);
    void onSetRateTempLeft(double);
    void onSetRateTempRight(double);
    void onSetRatePermLeft(double);
    void onSetRatePermRight(double);
    void onSetTooltips(int pos);
    void onSetSkin(int);
    void onSetScheme(int);
    void onUpdateSchemes();
    void onSetPositionDisplay(int);
    void onSetPositionDisplay(double);
    void onSetAllowTrackLoadToPlayingDeck(int);
    void onSetCueDefault(int);
    void onSetCueRecall(int);
    void onSetRateRamp(bool);
    void onSetRateRampSensitivity(int);
    void onSetLocale(int);
    void onSetStartInFullscreen(int index);

    void onNumDecksChanged(double);
    void onNumSamplersChanged(double);
    
    void onUpdateSpeedAutoReset(int);

  private:
    void notifyRebootNecessary();
    bool checkSkinResolution(QString skin);

    ConfigObject<ConfigValue>* m_pConfig;
    ControlObject* m_pControlPositionDisplay;
    ControlObjectSlave* m_pNumDecks;
    ControlObjectSlave* m_pNumSamplers;
    QList<ControlObjectSlave*> m_cueControls;
    QList<ControlPotmeter*>     m_rateControls;
    QList<ControlObjectSlave*> m_rateDirControls;
    QList<ControlObjectSlave*> m_rateRangeControls;
    QList<ControlObjectSlave*> m_keylockModeControls;
    MixxxMainWindow *m_mixxx;
    SkinLoader* m_pSkinLoader;
    PlayerManager* m_pPlayerManager;

    int m_iNumConfiguredDecks;
    int m_iNumConfiguredSamplers;
    
    int m_speedAutoReset;
    int m_keylockMode;
};

#endif
