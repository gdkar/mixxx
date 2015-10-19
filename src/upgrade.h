/***************************************************************************
                          upgrade.h  -  description
                             -------------------
    begin                : Mon Apr 13 2009
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
_Pragma("once")
#include <QObject>
#include "configobject.h"
class Upgrade : public QObject
{
    Q_OBJECT
    public:
        Upgrade(QObject*pParent=nullptr);
        virtual ~Upgrade();
        ConfigObject<ConfigValue>* versionUpgrade(QString settingsPath);
        static QString mixxx17HomePath();
        bool isFirstRun() const;
        bool isUpgraded() const;
        bool rescanLibrary() const;
    private:
        bool askReanalyzeBeats();
        bool askReScanLibrary();
        bool m_bFirstRun      = false;
        bool m_bUpgraded      = false;
        bool m_bRescanLibrary = false;
};
