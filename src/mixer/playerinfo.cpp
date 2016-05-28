/***************************************************************************
                      playerinfo.cpp  -  Helper class to have easy access
                                         to a lot of data (singleton)
                             -------------------
    copyright            : (C) 2007 by Wesley Stessens
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mixer/playerinfo.h"

#include <QMutexLocker>

#include "controlobject.h"
#include "engine/enginechannel.h"
#include "engine/enginexfader.h"
#include "mixer/playermanager.h"

static const int kPlayingDeckUpdateIntervalMillis = 2000;
static PlayerInfo* m_pPlayerInfo = NULL;

PlayerInfo::PlayerInfo()
        : m_pCOxfader(new ControlObjectSlave("[Master]","crossfader", this)),
          m_currentlyPlayingDeck(-1) {
    startTimer(kPlayingDeckUpdateIntervalMillis);
}
PlayerInfo::~PlayerInfo()
{
    m_loadedTrackMap.clear();
    clearControlCache();
}
// static
PlayerInfo& PlayerInfo::instance()
{
    if (!m_pPlayerInfo)
        m_pPlayerInfo = new PlayerInfo();
    return *m_pPlayerInfo;
}
// static
void PlayerInfo::destroy()
{
    delete m_pPlayerInfo;
}
TrackPointer PlayerInfo::getTrackInfo(const QString& group) {
    QMutexLocker locker(&m_mutex);
    return m_loadedTrackMap.value(group);
}
void PlayerInfo::setTrackInfo(const QString& group, const TrackPointer& track) {
    TrackPointer pOld;
    { // Scope
        QMutexLocker locker(&m_mutex);
        pOld = m_loadedTrackMap.value(group);
        m_loadedTrackMap.insert(group, track);
    }
    if (pOld)
        emit(trackUnloaded(group, pOld));
    emit(trackLoaded(group, track));
}
bool PlayerInfo::isTrackLoaded(const TrackPointer& pTrack) const {
    QMutexLocker locker(&m_mutex);
    QMapIterator<QString, TrackPointer> it(m_loadedTrackMap);
    while (it.hasNext()) {
        it.next();
        if (it.value() == pTrack) {
            return true;
        }
    }
    return false;
}

QMap<QString, TrackPointer> PlayerInfo::getLoadedTracks() {
    QMutexLocker locker(&m_mutex);
    QMap<QString, TrackPointer> ret = m_loadedTrackMap;
    return ret;
}

bool PlayerInfo::isFileLoaded(const QString& track_location) const {
    QMutexLocker locker(&m_mutex);
    QMapIterator<QString, TrackPointer> it(m_loadedTrackMap);
    while (it.hasNext()) {
        it.next();
        auto pTrack = it.value();
        if (pTrack) {
            if (pTrack->getLocation() == track_location) {
                return true;
            }
        }
    }
    return false;
}
void PlayerInfo::timerEvent(QTimerEvent* pTimerEvent) {
    Q_UNUSED(pTimerEvent);
    updateCurrentPlayingDeck();
}
void PlayerInfo::updateCurrentPlayingDeck() {
    QMutexLocker locker(&m_mutex);
    double maxVolume = 0;
    int maxDeck = -1;
    for (int i = 0; i < (int)PlayerManager::numDecks(); ++i) {
        auto  pDc = getDeckControls(i);
        if (pDc->m_play.get() == 0.0) {
            continue;
        }
        if (pDc->m_pregain.get() <= 0.5) {
            continue;
        }
        double fvol = pDc->m_volume.get();
        if (fvol == 0.0)
            continue;
        double xfl, xfr;
        EngineXfader::getXfadeGains(m_pCOxfader->get(), 1.0, 0.0, false, false,
                                    &xfl, &xfr);

        int orient = pDc->m_orientation.get();
        double xfvol;
        if (orient == EngineChannel::LEFT) {
            xfvol = xfl;
        } else if (orient == EngineChannel::RIGHT) {
            xfvol = xfr;
        } else {
            xfvol = 1.0;
        }
        auto dvol = fvol * xfvol;
        if (dvol > maxVolume) {
            maxDeck = i;
            maxVolume = dvol;
        }
    }
    if (maxDeck != m_currentlyPlayingDeck) {
        m_currentlyPlayingDeck = maxDeck;
        locker.unlock();
        emit(currentPlayingDeckChanged(maxDeck));
        emit(currentPlayingTrackChanged(getCurrentPlayingTrack()));
    }
}

int PlayerInfo::getCurrentPlayingDeck() {
    QMutexLocker locker(&m_mutex);
    return m_currentlyPlayingDeck;
}

TrackPointer PlayerInfo::getCurrentPlayingTrack() {
    auto deck = getCurrentPlayingDeck();
    if (deck >= 0)
        return getTrackInfo(PlayerManager::groupForDeck(deck));
    return TrackPointer();
}
PlayerInfo::DeckControls* PlayerInfo::getDeckControls(int i) {
    if (m_deckControlList.count() == i) {
        auto group = PlayerManager::groupForDeck(i);
        m_deckControlList.append(new DeckControls(group));
    }
    return m_deckControlList[i];
}
void PlayerInfo::clearControlCache() {
    for (int i = 0; i < m_deckControlList.count(); ++i)
        delete m_deckControlList[i];
    m_deckControlList.clear();
}
