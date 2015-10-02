_Pragma("once")
#include <vector>
#include <utility>
#include <memory>
#include <QList>
#include <QSqlDatabase>
#include <QRegExp>
#include <QString>
#include <QStringList>

#include "trackinfoobject.h"
#include "proto/keys.pb.h"
#include "util/assert.h"

QVariant getTrackValueForColumn(const TrackPointer& pTrack, const QString& column);

class QueryNode {
  public:
    QueryNode(const QueryNode&) = delete; // prevent copying
    virtual ~QueryNode();
    virtual bool match(const TrackPointer& pTrack) const = 0;
    virtual QString toSql() const = 0;
  protected:
    QueryNode();
    static QString concatSqlClauses(const QStringList& sqlClauses, const QString& sqlConcatOp);
};
class GroupNode : public QueryNode {
  public:
    void addNode(std::unique_ptr<QueryNode> pNode);
  protected:
    // NOTE(uklotzde): std::vector is more suitable (efficiency)
    // than a QList for a private member. And QList from Qt 4
    // does not support std::unique_ptr yet.
    std::vector<std::unique_ptr<QueryNode>> m_nodes;
};
class OrNode : public GroupNode {
  public:
    bool match(const TrackPointer& pTrack) const override;
    QString toSql() const override;
};
class AndNode : public GroupNode {
  public:
    bool match(const TrackPointer& pTrack) const override;
    QString toSql() const override;
};
class NotNode : public QueryNode {
  public:
    explicit NotNode(std::unique_ptr<QueryNode> pNode);
    bool match(const TrackPointer& pTrack) const override;
    QString toSql() const override;
  private:
    std::unique_ptr<QueryNode> m_pNode;
};
class TextFilterNode : public QueryNode {
  public:
    TextFilterNode(const QSqlDatabase& database,
                   const QStringList& sqlColumns,
                   const QString& argument);
    bool match(const TrackPointer& pTrack) const override;
    QString toSql() const override;
  private:
    QSqlDatabase m_database;
    QStringList m_sqlColumns;
    QString m_argument;
};
class NumericFilterNode : public QueryNode {
  public:
    NumericFilterNode(const QStringList& sqlColumns, const QString& argument);
    bool match(const TrackPointer& pTrack) const override;
    QString toSql() const override;
  protected:
    // Single argument constructor for that does not call init()
    explicit NumericFilterNode(const QStringList& sqlColumns);
    // init() must always be called in the constructor of the
    // most derived class directly, because internally it calls
    // the virtual function parse() that will be overridden by
    // derived classes.
    void init(QString argument);
  private:
    virtual double parse(const QString& arg, bool *ok);
    QStringList m_sqlColumns;
    bool m_bOperatorQuery = false;
    QString m_operator;
    double m_dOperatorArgument = 0.0;
    bool m_bRangeQuery  = false;
    double m_dRangeLow  = std::numeric_limits<double>::min();
    double m_dRangeHigh = std::numeric_limits<double>::max();
};
class DurationFilterNode : public NumericFilterNode {
  public:
    DurationFilterNode(const QStringList& sqlColumns, const QString& argument);
  private:
    double parse(const QString& arg, bool* ok) override;
};
class KeyFilterNode : public QueryNode {
  public:
    KeyFilterNode(mixxx::track::io::key::ChromaticKey key, bool fuzzy);
    bool match(const TrackPointer& pTrack) const override;
    QString toSql() const override;
  private:
    QList<mixxx::track::io::key::ChromaticKey> m_matchKeys;
};
class SqlNode : public QueryNode {
  public:
    explicit SqlNode(QString sqlExpression);
    bool match(const TrackPointer& pTrack) const override;
    QString toSql() const override;
  private:
    QString m_sql;
};
