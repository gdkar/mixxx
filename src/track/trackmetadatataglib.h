#ifndef TRACKMETADATATAGLIB_H
#define TRACKMETADATATAGLIB_H

#include <taglib/apetag.h>
#include <taglib/id3v2tag.h>
#include <taglib/xiphcomment.h>
#include <taglib/mp4tag.h>

#include <QImage>

#include "track/trackmetadata.h"

namespace mixxx {
// Read both track metadata and cover art of supported file types.
// Both parameters are optional and might be NULL.
bool readTrackMetadataAndCoverArtFromFile(TrackMetadata* pTrackMetadata, QImage* pCoverArt, QString fileName);
// Low-level tag read/write functions are exposed only for testing purposes!
void readTrackMetadataFromID3v2Tag(TrackMetadata* pTrackMetadata, const TagLib::ID3v2::Tag& tag);
void readTrackMetadataFromAPETag(TrackMetadata* pTrackMetadata, const TagLib::APE::Tag& tag);
void readTrackMetadataFromXiphComment(TrackMetadata* pTrackMetadata,const TagLib::Ogg::XiphComment& tag);
void readTrackMetadataFromMP4Tag(TrackMetadata* pTrackMetadata, const TagLib::MP4::Tag& tag);
} //namespace mixxx

#endif
