_Pragma("once")
#include <QAbstractTableModel>
#include <QVariant>
#include <QVector>
#include <QHash>
#include <QList>
#include <QModelIndex>
#include <QString>

#include "util/stat.h"

class StatModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    enum class StatColumn {
        Name= 0,
        Count,
        Type,
        Units,
        Sum,
        Min,
        Max,
        Mean,
        Variance,
        StdDev
    };
    Q_ENUM(StatColumn);
    explicit StatModel(QObject* pParent=nullptr);
    virtual ~StatModel();
  public slots:
    void statUpdated(const Stat& stat);
    ////////////////////////////////////////////////////////////////////////////
    // QAbstractItemModel methods
    ////////////////////////////////////////////////////////////////////////////
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    bool setHeaderData(int section, Qt::Orientation orientation,const QVariant& value, int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation,int role = Qt::DisplayRole) const;
  private:
    QVector<QHash<int, QVariant> > m_headerInfo;
    QList<Stat> m_stats;
    QHash<QString, int> m_statNameToRow;
};
