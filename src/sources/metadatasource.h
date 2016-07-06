#ifndef MIXXX_METADATASOURCE_H
#define MIXXX_METADATASOURCE_H

#include <QImage>

#include "track/trackmetadata.h"

namespace mixxx {

// Interface for parsing track metadata and cover art.
class MetadataSource {
public:
    // Read both track metadata and cover art at once, because this
    // is should be the most common use case. Both parameters are
    // output parameters and might be nullptr if their result is not
    // needed.
    virtual bool parseTrackMetadataAndCoverArt(
            TrackMetadata* pTrackMetadata,
            QImage* pCoverArt) const = 0;
protected:
    virtual ~MetadataSource() = default;
};

} //namespace mixxx

#endif // MIXXX_METADATASOURCE_H
