// cue.cpp
// Created 10/26/2009 by RJ Ryan (rryan@mit.edu)

#include <QMutexLocker>
#include <QtDebug>

#include "library/dao/cue.h"
#include "util/assert.h"

Cue::~Cue() {
    qDebug() << "~Cue()" << m_iId;
}

Cue::Cue(TrackId trackId)
:m_trackId(trackId)
{
}
Cue::Cue(int id, TrackId trackId, Cue::CueType type, int position, int length,
         int hotCue, QString label)
        : m_bDirty(false),
          m_iId(id),
          m_trackId(trackId),
          m_type(type),
          m_iPosition(position),
          m_iLength(length),
          m_iHotCue(hotCue),
          m_label(label)
{
}
int Cue::getId() const
{
    return m_iId.load();
}
void Cue::setId(int cueId)
{
    if(m_iId.exchange(cueId)!=cueId)
    {
      m_bDirty = true;
      emit updated();
    }
}
TrackId Cue::getTrackId() const
{
    return m_trackId.load();
}
void Cue::setTrackId(TrackId trackId)
{
    if(m_trackId.exchange(trackId)!=trackId)
    {
      m_bDirty = true;
      emit updated();
    }
}
Cue::CueType Cue::getType() const
{
    return m_type.load();
}
void Cue::setType(Cue::CueType type)
{
    if(m_type.exchange(type) != type)
    {
      m_bDirty = true;
      emit(updated());
    }
}
int Cue::getPosition() const
{
    return m_iPosition.load();
}
void Cue::setPosition(int position)
{
    DEBUG_ASSERT_AND_HANDLE(position % 2 == 0) {return;}
    if(m_iPosition.exchange(position)!=position)
    {
      m_bDirty = true;
      emit(updated());
    }
}
int Cue::getLength() const
{
    return m_iLength.load();
}
void Cue::setLength(int length)
{
    DEBUG_ASSERT_AND_HANDLE(length % 2 == 0) {return;}
    if(m_iLength.exchange(length)!=length)
    {
      m_bDirty = true;
      emit(updated());
    }
}
int Cue::getHotCue() const
{
    return m_iHotCue.load();
}
void Cue::setHotCue(int hotCue)
{
    // TODO(XXX) enforce uniqueness?
    if(m_iHotCue.exchange(hotCue)!=hotCue)
    {
      m_bDirty = true;
      emit(updated());
    }
}
QString Cue::getLabel() const
{
    return m_label;
}
void Cue::setLabel(const QString label)
{
    auto old_label = label;
    m_label.swap(old_label);
    if(old_label != label)
    {
      m_bDirty = true;
      emit(updated());
    }
}
bool Cue::isDirty() const
{
    return m_bDirty.load();;
}
void Cue::setDirty(bool dirty)
{
    m_bDirty.store(dirty);
}
