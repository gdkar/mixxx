// cue.h
// Created 10/26/2009 by RJ Ryan (rryan@mit.edu)

#ifndef CUE_H
#define CUE_H

#include <QObject>
#include <QMutex>
#include <QSharedPointer>
#include <QColor>

#include "track/trackid.h"

class CueDAO;
class Track;

class Cue : public QObject, public QEnableSharedFromThis<Cue> {
  Q_OBJECT
  Q_PROPERTY(bool dirty READ isDirty WRITE setDirty NOTIFY dirtyChanged)
  Q_PROPERTY(CueType type READ getType WRITE setType NOTIFY typeChanged)
  Q_PROPERTY(int position READ getPosition WRITE setPosition NOTIFY positionChanged)
  Q_PROPERTY(int length READ getLength WRITE setLength NOTIFY lengthChanged)
  Q_PROPERTY(int hotCue READ getHotCue WRITE setHotCue NOTIFY hotCueChanged)
  Q_PROPERTY(QString label READ getLabel WRITE setLabel NOTIFY labelChanged)
  Q_PROPERTY(TrackId trackId READ getTrackId WRITE setTrackId NOTIFY trackIdChanged)
  Q_PROPERTY(QColor color READ getColor WRITE setColor NOTIFY colorChanged)
  public:
    enum CueType {
        INVALID = 0,
        CUE,
        LOAD,
        BEAT,
        LOOP,
        JUMP,
    };
    Q_ENUM(CueType);

    virtual ~Cue();

    bool isDirty() const;
    int getId() const;
    TrackId getTrackId() const;

    CueType getType() const;
    void setType(CueType type);

    int getPosition() const;
    void setPosition(int position);

    int getLength() const;
    void setLength(int length);

    int getHotCue() const;
    void setHotCue(int hotCue);

    QString getLabel() const;
    void setLabel(QString label);

    QColor getColor() const;
    void setColor(QColor color);

  signals:
    void updated();
    void dirtyChanged(bool);
    void typeChanged(CueType);
    void positionChanged(int);
    void lengthChanged(int);
    void hotCueChanged(int);
    void labelChanged(QString);
    void colorChanged(QColor);
    void trackIdChanged(TrackId);
  private:
    explicit Cue(TrackId trackId);
    explicit Cue(QObject *pParent = nullptr);
    Cue(int id, TrackId trackId, CueType type, int position, int length,
        int hotCue, QString label, QColor color);
    void setDirty(bool dirty);
    void setId(int id);
    void setTrackId(TrackId trackId);

    mutable QMutex m_mutex;

    bool m_bDirty;
    int m_iId;
    TrackId m_trackId;
    CueType m_type;
    int m_iPosition;
    int m_iLength;
    int m_iHotCue;
    QString m_label;
    QColor m_color;

    friend class Track;
    friend class CueDAO;
};

typedef QSharedPointer<Cue> CuePointer;

#endif /* CUE_H */
