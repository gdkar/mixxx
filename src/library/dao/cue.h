// cue.h
// Created 10/26/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QObject>
#include <QMutex>
#include <atomic>
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
    Q_PROPERTY(int id READ getId WRITE setId NOTIFY updated);
    Q_PROPERTY(QString label READ getLabel WRITE setLabel NOTIFY updated);
    Q_PROPERTY(int hotcue READ getHotCue WRITE setHotCue NOTIFY updated);
    Q_PROPERTY(int length READ getLength WRITE setLength NOTIFY updated);
    Q_PROPERTY(int position READ getPosition WRITE setPosition NOTIFY updated);
    Q_PROPERTY(CueType type READ getType WRITE setType NOTIFY updated);
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
    std::atomic<bool>    m_bDirty{false};
    std::atomic<int>     m_iId{-1};
    std::atomic<TrackId> m_trackId{};
    std::atomic<CueType> m_type{INVALID};
    std::atomic<int> m_iPosition{-1};
    std::atomic<int> m_iLength{0};
    std::atomic<int> m_iHotCue{-1};
    QString m_label;
    friend class TrackInfoObject;
    friend class CueDAO;
};
