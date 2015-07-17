// cue.cpp
// Created 10/26/2009 by RJ Ryan (rryan@mit.edu)

#include <QMutexLocker>
#include <QtDebug>

#include "library/dao/cue.h"
#include "util/assert.h"

Cue::~Cue() {
    qDebug() << "~Cue()" << m_iId;
}

Cue::Cue(int trackId)
        : m_bDirty(false),
          m_iId(-1),
          m_iTrackId(trackId),
          m_type(INVALID),
          m_iPosition(-1),
          m_iLength(0),
          m_iHotCue(-1),
          m_label("") {
    //qDebug() << "Cue(int)";
}
Cue::Cue(int id, int trackId, Cue::CueType type, int position, int length,
         int hotCue, QString label)
        : m_bDirty(false),
          m_iId(id),
          m_iTrackId(trackId),
          m_type(type),
          m_iPosition(position),
          m_iLength(length),
          m_iHotCue(hotCue),
          m_label(label) {
    //qDebug() << "Cue(...)";
}
int Cue::getId() {return m_iId.load();}
void Cue::setId(int cueId) {
  if(m_iId.exchange(cueId)!=cueId){
    m_bDirty.store(true);
    emit(updated());
  }
}
int Cue::getTrackId() {return m_iTrackId.load();}
void Cue::setTrackId(int trackId) {
  if(m_iTrackId.exchange(trackId)!=trackId){
    m_bDirty.store(true);
    emit(updated());
  }
}
Cue::CueType Cue::getType() {return m_type.load();}
void Cue::setType(Cue::CueType type) {
    if(m_type.exchange(type)!=type){
      m_bDirty.store(true);
      emit(updated());
    }
}
int Cue::getPosition() {return m_iPosition.load();}
void Cue::setPosition(int position) {
    DEBUG_ASSERT_AND_HANDLE(position % 2 == 0) {return;}
    if(m_iPosition.exchange(position)!=position){
      m_iPosition = position;
      m_bDirty = true;
      emit(updated());
    }
}
int Cue::getLength() {return m_iLength.load();}
void Cue::setLength(int length) {
    DEBUG_ASSERT_AND_HANDLE(length % 2 == 0) {return;}
    if(m_iLength.exchange(length)!=length){
      m_bDirty.store(true);
      emit updated();
    }
}
int Cue::getHotCue() {return m_iHotCue.load();}
void Cue::setHotCue(int hotCue) {
    if(m_iHotCue.exchange(hotCue)!=hotCue){
      m_bDirty.store( true);
      emit(updated());
    }
}
const QString Cue::getLabel() {
    QString label(m_label);
    return label;
}
void Cue::setLabel(const QString label) {
    //qDebug() << "setLabel()" << m_label << "-" << label;
    QString oldLabel(label);
    m_label.swap(oldLabel);
    if(oldLabel!=label){
      m_bDirty.store( true);
      emit(updated());
    }
}
bool Cue::isDirty() {return m_bDirty.load();}
void Cue::setDirty(bool dirty) {m_bDirty.store(dirty);}
