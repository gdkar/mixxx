// cratetablemodel.h
// Created 10/25/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include "library/basesqltablemodel.h"
#include "library/dao/cratedao.h"

class CrateTableModel : public BaseSqlTableModel {
    Q_OBJECT
  public:
    CrateTableModel(QObject* parent, TrackCollection* pTrackCollection);
    virtual ~CrateTableModel();
    void setTableModel(int crateId=-1);
    int getCrate() const;
    // From TrackModel
    bool isColumnInternal(int column);
    void removeTracks(const QModelIndexList& indices);
    bool addTrack(const QModelIndex &index, QString location);
    // Returns the number of unsuccessful track additions
    int addTracks(const QModelIndex& index, const QList<QString>& locations);
    TrackModel::CapabilitiesFlags getCapabilities() const;
  private:
    int m_iCrateId;
    CrateDAO& m_crateDAO;
};
