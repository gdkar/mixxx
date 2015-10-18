#include <limits>
#include <QMetaEnum>
#include "util/statmodel.h"
#include "util/math.h"

StatModel::StatModel(QObject* pParent)
        : QAbstractTableModel(pParent)
{
    auto colEnum = QMetaEnum::fromType<StatColumn>();
    for(auto i = 0; i < colEnum.keyCount();i++)
    {
      setHeaderData(colEnum.value(i),Qt::Horizontal,tr(colEnum.key(i)));
    }
}

StatModel::~StatModel() = default;

void StatModel::statUpdated(const Stat& stat) {
    auto it = m_statNameToRow.find(stat.m_tag);
    if ( m_statNameToRow.contains(stat.m_tag))
    {
      auto row     = m_statNameToRow.value(stat.m_tag);
      m_stats[row] = stat;
      auto left = index(row,0);
      auto right= index(row,columnCount() - 1);
      emit dataChanged(left,right);
    }
    else
    {
        beginInsertRows(QModelIndex(), m_stats.size(),m_stats.size());
        m_statNameToRow[stat.m_tag] = m_stats.size();
        m_stats.append(stat);
        endInsertRows();
    }
}
int StatModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_stats.size();
}

int StatModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    auto colEnum = QMetaEnum::fromType<StatColumn>();
    return colEnum.keyCount();
}

QVariant StatModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole)) return QVariant();
    auto row    = index.row();
    auto column = index.column();
    if (row < 0 || row >= m_stats.size()) return QVariant();

    auto& stat = m_stats.at(row);
    auto value = QString{};
    auto col = static_cast<StatColumn>(column);
    switch (col) {
        case StatColumn::Name:
            return stat.m_tag;
        case StatColumn::Type:
            return stat.m_type;
        case StatColumn::Count:
            return stat.m_report_count;
        case StatColumn::Sum:
            return stat.m_sum;
        case StatColumn::Min:
            return std::numeric_limits<double>::max() == stat.m_min ?
                    QVariant("XXX") : QVariant(stat.m_min);
        case StatColumn::Max:
            return std::numeric_limits<double>::min() == stat.m_max ?
                    QVariant("XXX") : QVariant(stat.m_max);
        case StatColumn::Mean:
            return stat.m_report_count > 0 ?
                    QVariant(stat.m_sum / stat.m_report_count) : QVariant("XXX");
        case StatColumn::Variance:
            return stat.variance();
        case StatColumn::StdDev:
            return sqrt(stat.variance());
        case StatColumn::Units:
            return stat.valueUnits();
    }
    return QVariant();
}

bool StatModel::setHeaderData(int section,
                              Qt::Orientation orientation,
                              const QVariant& value,
                              int role) {
    int numColumns = columnCount();
    if (section < 0 || section >= numColumns) {
        return false;
    }

    if (orientation != Qt::Horizontal) {
        // We only care about horizontal headers.
        return false;
    }

    if (m_headerInfo.size() != numColumns) {
        m_headerInfo.resize(numColumns);
    }

    m_headerInfo[section][role] = value;
    emit(headerDataChanged(orientation, section, section));
    return true;
}

QVariant StatModel::headerData(int section,
                               Qt::Orientation orientation,
                               int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        QVariant headerValue = m_headerInfo.value(section).value(role);
        if (!headerValue.isValid()) {
            // Try EditRole if DisplayRole wasn't present
            headerValue = m_headerInfo.value(section).value(Qt::EditRole);
        }
        if (!headerValue.isValid()) {
            headerValue = QVariant(section).toString();
        }
        return headerValue;
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
