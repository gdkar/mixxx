_Pragma("once")
#include <QtDebug>
#include <QString>
#include <QtSql>

#define LOG_FAILED_QUERY(query) qDebug() << __FILE__ << __LINE__ << "FAILED QUERY [" \
    << (query).executedQuery() << "]" << (query).lastError()

class ScopedTransaction {
  public:
    explicit ScopedTransaction(QSqlDatabase& database);
    virtual ~ScopedTransaction();
    bool active() const;
    bool transaction();
    bool commit();
    bool rollback();
  private:
    QSqlDatabase& m_database;
    bool m_active = false;
};
class FieldEscaper {
  public:
    FieldEscaper(const QSqlDatabase& database);
    virtual ~FieldEscaper();
    // Escapes a string for use in a SQL query by wrapping with quotes and
    // escaping embedded quote characters.
    QString escapeString(QString escapeString) const;
    QStringList escapeStrings(QStringList escapeStrings) const;
    void escapeStringsInPlace(QStringList* pEscapeStrings) const;
    // Escapes a string for use in a LIKE operation by prefixing instances of
    // LIKE wildcard characters (% and _) with escapeCharacter. This allows the
    // caller to then attach wildcard characters to the string. This does NOT
    // escape the string in the same way that escapeString() does.
    QString escapeStringForLike(QString escapeString, QChar escapeCharacter) const;
  private:
    const QSqlDatabase& m_database;
    mutable QSqlField m_stringField{"string",QVariant::String};
};
