#ifndef TRACKID_H
#define TRACKID_H


#include "util/dbid.h"


class TrackId: public DbId {
public:
    // Inherit constructors from base class
    using DbId::DbId;
    constexpr TrackId() = default;
    constexpr TrackId(const TrackId&) = default;
    constexpr TrackId(TrackId&&) noexcept = default;
    constexpr TrackId&operator=(const TrackId&) = default;
    constexpr TrackId&operator=(TrackId&&) noexcept = default;
};

Q_DECLARE_METATYPE(TrackId)

#endif // TRACKID_H
