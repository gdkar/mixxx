_Pragma("once")
#include "util/dbid.h"
class TrackId: public DbId
{
public:
    // Inherit constructors from base class
    using DbId::DbId;
};
Q_DECLARE_METATYPE(TrackId);
Q_DECLARE_TYPEINFO(TrackId,Q_PRIMITIVE_TYPE);
