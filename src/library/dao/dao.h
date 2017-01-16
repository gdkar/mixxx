// dao.h
// Created 10/22/2009 by RJ Ryan (rryan@mit.edu)

#ifndef DAO_H
#define DAO_H

class DAO {
public:
    virtual ~DAO() = 0;
    virtual void initialize() = 0;
};
inline DAO::~DAO() {}
#endif /* DAO_H */
