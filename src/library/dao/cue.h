// cue.h
// Created 10/26/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QObject>
#include <QMutex>
#include <QSharedPointer>

#include "track/trackid.h"

class CueDAO;
class TrackInfoObject;

class Cue : public QObject {
    Q_OBJECT

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
    void setLabel(const QString label);

  signals:
    void updated();

  private:
    explicit Cue(TrackId trackId);
    Cue(int id, TrackId trackId, CueType type, int position, int length,int hotCue, QString label);
    void setDirty(bool dirty);
    void setId(int id);
    void setTrackId(TrackId trackId);
    mutable QMutex m_mutex;

    bool m_bDirty{false};
    int m_iId{-1};
    TrackId m_trackId{-1};
    CueType m_type{CueType::INVALID};
    int m_iPosition{-1};
    int m_iLength{-1};
    int m_iHotCue{-1};
    QString m_label;

    friend class TrackInfoObject;
    friend class CueDAO;
};

typedef QSharedPointer<Cue> CuePointer;
