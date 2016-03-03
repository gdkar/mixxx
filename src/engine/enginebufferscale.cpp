/***************************************************************************
                          enginebufferscale.cpp  -  description
                             -------------------
    begin                : Sun Apr 13 2003
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

#include "engine/enginebufferscale.h"
#include "util/defs.h"
#include "util/sample.h"

EngineBufferScale::EngineBufferScale(QObject *pParent )
        : QObject(pParent)
{
    if(pParent)
        m_pReadAheadManager = pParent->findChild<ReadAheadManager*>();
}
EngineBufferScale::EngineBufferScale(ReadAheadManager *pRAMan, QObject *pParent)
    : QObject(pParent)
    , m_pReadAheadManager(pRAMan)
{
}
EngineBufferScale::~EngineBufferScale() = default;
