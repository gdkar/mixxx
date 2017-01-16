#ifndef DAO_H
#define DAO_H

#include <QSqlDatabase>

class DAO {
  public:
    virtual void initialize(const QSqlDatabase& database) = 0;
    virtual ~DAO() = 0;
};
inline DAO::~DAO() {}
#endif /* DAO_H */
