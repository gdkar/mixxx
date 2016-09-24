// cue.cpp
// Created 10/26/2009 by RJ Ryan (rryan@mit.edu)

#include <QMutexLocker>
#include <QtDebug>

#include "library/dao/cue.h"
#include "util/assert.h"

namespace {
    const QColor kDefaultColor = QColor("#FF0000");
}

Cue::~Cue() {
    qDebug() << "~Cue()" << m_iId;
}
Cue::Cue(QObject *pParent)
        : QObject(pParent),
          m_bDirty(false),
          m_iId(-1),
          m_trackId(TrackId(-1)),
          m_type(INVALID),
          m_iPosition(-1),
          m_iLength(0),
          m_iHotCue(-1),
          m_label(),
          m_color(kDefaultColor) {
}
Cue::Cue(TrackId trackId)
        : m_bDirty(false),
          m_iId(-1),
          m_trackId(trackId),
          m_type(INVALID),
          m_iPosition(-1),
          m_iLength(0),
          m_iHotCue(-1),
          m_label(),
          m_color(kDefaultColor) {
}


Cue::Cue(int id, TrackId trackId, Cue::CueType type, int position, int length,
         int hotCue, QString label, QColor color)
        : m_bDirty(false),
          m_iId(id),
          m_trackId(trackId),
          m_type(type),
          m_iPosition(position),
          m_iLength(length),
          m_iHotCue(hotCue),
          m_label(label),
          m_color(color)
{
    connect(this, &Cue::trackIdChanged, this, &Cue::updated);
    connect(this, &Cue::typeChanged, this, &Cue::updated);
    connect(this, &Cue::positionChanged, this, &Cue::updated);
    connect(this, &Cue::lengthChanged, this, &Cue::updated);
    connect(this, &Cue::hotCueChanged, this, &Cue::updated);
    connect(this, &Cue::labelChanged, this, &Cue::updated);
    connect(this, &Cue::colorChanged, this, &Cue::updated);
}

int Cue::getId() const {
    QMutexLocker lock(&m_mutex);
    return m_iId;
}

void Cue::setId(int cueId) {
    auto changed = false;
    {
        QMutexLocker lock(&m_mutex);
        changed = m_iId != cueId;
        m_iId = cueId;
    }
    if(changed) {
        setDirty(true);
        emit updated();
    }
}

TrackId Cue::getTrackId() const {
    QMutexLocker lock(&m_mutex);
    return m_trackId;
}

void Cue::setTrackId(TrackId trackId) {
    auto changed = false;
    {
        QMutexLocker lock(&m_mutex);
        changed = (m_trackId != trackId);
        m_trackId = trackId;
    }
    if(changed) {
        setDirty(true);
        emit trackIdChanged(trackId);
    }
}

Cue::CueType Cue::getType() const {
    QMutexLocker lock(&m_mutex);
    return m_type;
}

void Cue::setType(Cue::CueType type) {
    auto changed = false;
    {
        QMutexLocker lock(&m_mutex);
        changed = m_type != type;
        m_type = type;
    }
    if(changed) {
        setDirty(true);
        emit typeChanged(type);
    }
}

int Cue::getPosition() const {
    QMutexLocker lock(&m_mutex);
    return m_iPosition;
}

void Cue::setPosition(int position) {
    DEBUG_ASSERT_AND_HANDLE(position % 2 == 0) {
        return;
    }
    auto changed = false;
    {
        QMutexLocker lock(&m_mutex);
        changed = m_iPosition != position;
        m_iPosition= position;
    }
    if(changed) {
        setDirty(true);
        emit positionChanged(position);
    }
}

int Cue::getLength() const {
    QMutexLocker lock(&m_mutex);
    return m_iLength;
}

void Cue::setLength(int length) {
    DEBUG_ASSERT_AND_HANDLE(length % 2 == 0) {
        return;
    }
    auto changed = false;
    {
        QMutexLocker lock(&m_mutex);
        changed = m_iLength != length;
        m_iLength= length;
    }
    if(changed) {
        setDirty(true);
        emit lengthChanged(length);
    }
}

int Cue::getHotCue() const {
    QMutexLocker lock(&m_mutex);
    return m_iHotCue;
}

void Cue::setHotCue(int hotCue) {
    auto changed = false;
    {
        QMutexLocker lock(&m_mutex);
        changed = m_iHotCue != hotCue;
        m_iHotCue= hotCue;
    }
    if(changed) {
        setDirty(true);
        emit hotCueChanged(hotCue);
    }
}

QString Cue::getLabel() const {
    QMutexLocker lock(&m_mutex);
    return m_label;
}

void Cue::setLabel(const QString label) {
    auto changed = false;
    {
        QMutexLocker lock(&m_mutex);
        changed = m_label != label;
        m_label= label;
    }
    if(changed) {
        setDirty(true);
        emit labelChanged(label);
    }
}

QColor Cue::getColor() const {
    QMutexLocker lock(&m_mutex);
    return m_color;
}

void Cue::setColor(const QColor color) {
    auto changed = false;
    {
        QMutexLocker lock(&m_mutex);
        changed = m_color != color;
        m_color= color;
    }
    if(changed) {
        setDirty(true);
        emit colorChanged(color);
    }
}
bool Cue::isDirty() const {
    QMutexLocker lock(&m_mutex);
    return m_bDirty;
}
void Cue::setDirty(bool dirty) {
    auto changed = false;
    {
        QMutexLocker lock(&m_mutex);
        changed = m_bDirty != dirty;
        m_bDirty = dirty;
    }
    if(changed)
        emit dirtyChanged(dirty);
}
