/***************************************************************************
                        playerinfo.h  -  Helper class to have easy access
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

#ifndef PLAYERINFO_H
#define PLAYERINFO_H

#include <QObject>
#include <QMutex>
#include <QMap>
#include <QTimerEvent>
#include "trackinfoobject.h"
class ControlObjectSlave;
class DeckControls : public QObject{
  Q_OBJECT
    public:
        DeckControls(QString& group, QObject *pParent = nullptr);
        virtual ~DeckControls();
        ControlObjectSlave* m_play;
        ControlObjectSlave* m_pregain;
        ControlObjectSlave* m_volume;
        ControlObjectSlave* m_orientation;
};
class PlayerInfo : public QObject {
    Q_OBJECT
  public:
    static PlayerInfo& instance();
    static void destroy();
    TrackPointer getTrackInfo(const QString& group);
    void setTrackInfo(const QString& group, const TrackPointer& trackInfoObj);
    TrackPointer getCurrentPlayingTrack();
    QMap<QString, TrackPointer> getLoadedTracks();
    bool isTrackLoaded(const TrackPointer& pTrack) const;
    bool isFileLoaded(const QString& track_location) const;
  signals:
    void currentPlayingDeckChanged(int deck);
    void currentPlayingTrackChanged(TrackPointer pTrack);
    void trackLoaded(QString group, TrackPointer pTrack);
    void trackUnloaded(QString group, TrackPointer pTrack);
  private:

    void clearControlCache();
    void timerEvent(QTimerEvent* pTimerEvent);
    void updateCurrentPlayingDeck();
    int getCurrentPlayingDeck();
    DeckControls* getDeckControls(int i);
    PlayerInfo();
    virtual ~PlayerInfo();
    mutable QMutex m_mutex;
    ControlObjectSlave* m_pCOxfader;
    // QMap is faster than QHash for small count of elements < 50
    QMap<QString, TrackPointer> m_loadedTrackMap;
    int m_currentlyPlayingDeck;
    QList<DeckControls*> m_deckControlList;
    static PlayerInfo* m_pPlayerinfo;
};
#endif /* _PLAYERINFO_H_ */
