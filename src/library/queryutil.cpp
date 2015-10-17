#include "library/queryutil.h"

ScopedTransaction::ScopedTransaction(QSqlDatabase& database)
  : m_database(database)
{
  if(!transaction())
    qDebug() << "ERROR: could not start transaction on " << m_database.connectionName();
}
ScopedTransaction::~ScopedTransaction() 
{
  if(active()) rollback();
}
bool ScopedTransaction::active() const
{
  return m_active;
}
bool ScopedTransaction::transaction()
{
    if (active())
    {
        qDebug() << "WARNING: Transaction already active and received transaction() request on"
                  << m_database.connectionName();
        return false;
    }
    return (m_active = m_database.transaction());
}
bool ScopedTransaction::commit()
{
    if (!active())
    {
        qDebug() << "WARNING: commit() called on inactive transaction for"
                  << m_database.connectionName();
        return false;
    }
    auto result = m_database.commit();
    qDebug() << "Committing transaction on" << m_database.connectionName() << "result:" << result;
    m_active = false;
    return result;
}
bool ScopedTransaction::rollback()
{
    if (!m_active)
    {
        qDebug() << "WARNING: rollback() called on inactive transaction for"
                  << m_database.connectionName();
        return false;
    }
    auto result = m_database.rollback();
    qDebug() << "Rolling back transaction on" << m_database.connectionName() << "result:" << result;
    m_active = false;
    return result;
}
FieldEscaper::FieldEscaper(const QSqlDatabase& database)
        : m_database(database)
{
}
FieldEscaper::~FieldEscaper() = default;
QString FieldEscaper::escapeString(QString escapeString) const
{
    m_stringField.setValue(escapeString);
    return m_database.driver()->formatValue(m_stringField);
}
QStringList FieldEscaper::escapeStrings(QStringList escapeStrings) const
{
    auto result = escapeStrings;
    escapeStringsInPlace(&result);
    return result;
}
void FieldEscaper::escapeStringsInPlace(QStringList* pEscapeStrings) const
{
    QMutableStringListIterator it(*pEscapeStrings);
    while (it.hasNext()) it.setValue(escapeString(it.next()));
}
QString FieldEscaper::escapeStringForLike(QString escapeString, QChar escapeCharacter) const
{
    auto escapeCharacterStr = QString{escapeCharacter};
    // Replace instances of escapeCharacter with two escapeCharacters.
    auto result = escapeString.replace(escapeCharacter, escapeCharacterStr + escapeCharacterStr);
    // Replace instances of % or _ with $escapeCharacter%.
    if (escapeCharacter != '%') result = result.replace("%", escapeCharacterStr + "%");
    if (escapeCharacter != '_') result = result.replace("_", escapeCharacterStr + "_");
    return result;
}
