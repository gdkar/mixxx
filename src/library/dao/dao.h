// dao.h
// Created 10/22/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
class DAO {
    virtual void initialize() = 0;
    public:
    virtual ~DAO() = default;

};
