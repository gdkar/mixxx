/***************************************************************************
                          dlgpreflibrary.h  -  description
                             -------------------
    begin                : Thu Apr 17 2003
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

#ifndef DLGPREFLIBRARY_H
#define DLGPREFLIBRARY_H

#include <QStandardItemModel>
#include <QWidget>
#include <QFont>

#include "dialogs/ui_dlgpreflibrarydlg.h"
#include "configobject.h"
#include "library/library.h"
#include "preferences/dlgpreferencepage.h"

/**
  *@author Tue & Ken Haste Andersen
  */

class DlgPrefLibrary : public DlgPreferencePage, public Ui::DlgPrefLibraryDlg  {
    Q_OBJECT
  public:
    enum TrackLoadAction {
        LOAD_TRACK_DECK,  // Load track to next available deck.
        ADD_TRACK_BOTTOM, // Add track to Auto-DJ Queue (bottom).
        ADD_TRACK_TOP     // Add track to Auto-DJ Queue (top).
    };

    DlgPrefLibrary(QWidget *parent, ConfigObject<ConfigValue> *config,
                   Library *pLibrary);
    virtual ~DlgPrefLibrary();

  public slots:
    // Common preference page slots.
    void onUpdate();
    void onShow();
    void onHide();
    void onResetToDefaults();
    void onApply();
    void onCancel();

    // Dialog to browse for music file directory
    void onAddDir();
    void onRemoveDir();
    void onRelocateDir();
    void onExtraPlugins();

  signals:
    void apply();
    void scanLibrary();
    void requestAddDir(QString dir);
    void requestRemoveDir(QString dir, Library::RemovalType removalType);
    void requestRelocateDir(QString currentDir, QString newDir);
    void setTrackTableFont(const QFont& font);
    void setTrackTableRowHeight(int rowHeight);

  private slots:
    void onRowHeightValueChanged(int);
    void onSelectFont();

  private:
    void initialiseDirList();
    void setLibraryFont(const QFont& font);

    QStandardItemModel m_dirListModel;
    ConfigObject<ConfigValue>* m_pconfig;
    Library* m_pLibrary;
    bool m_baddedDirectory;
    QFont m_originalTrackTableFont;
    int m_iOriginalTrackTableRowHeight;
};

#endif
