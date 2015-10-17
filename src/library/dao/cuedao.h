// cuedao.h
// Created 10/26/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QMap>
#include <QSqlDatabase>

#include "trackinfoobject.h"
#include "library/dao/dao.h"

#define CUE_TABLE "cues"

class Cue;

class CueDAO : public DAO {
  public:
    CueDAO(QSqlDatabase& database);
    virtual ~CueDAO();
    void setDatabase(QSqlDatabase& database);
    void initialize();
    int cueCount();
    int numCuesForTrack(TrackId trackId);
    QList<Cue*> getCuesForTrack(TrackId trackId) const;
    bool deleteCuesForTrack(TrackId trackId);
    bool deleteCuesForTracks(const QList<TrackId>& trackIds);
    bool saveCue(Cue* cue);
    bool deleteCue(Cue* cue);
    // TODO(XXX) once we refer to all tracks by their id and TIO has a getId()
    // method the first parameter here won't be necessary.
    void saveTrackCues(TrackId trackId, TrackInfoObject*);
  private:
    Cue* cueFromRow(const QSqlQuery& query) const;
    QSqlDatabase& m_database;
    mutable QMap<int, Cue*> m_cues;
};
