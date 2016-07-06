#include "track/trackmetadatataglib.h"

#include "track/tracknumbers.h"

#include <cstdint>
#include <vector>
#include <deque>
#include <atomic>

extern "C"{
#   include <libavcodec/avcodec.h>
#   include <libavformat/avformat.h>
#   include <libavcodec/avcodec.h>
#   include <libswresample/swresample.h>
#   include <libavdevice/avdevice.h>
#   include <libavutil/avutil.h>
}
#include "util/assert.h"
#include "util/duration.h"
#include "util/memory.h"

// TagLib has full support for MP4 atom types since version 1.8
#define TAGLIB_HAS_MP4_ATOM_TYPES \
    (TAGLIB_MAJOR_VERSION > 1) || ((TAGLIB_MAJOR_VERSION == 1) && (TAGLIB_MINOR_VERSION >= 8))

// TagLib has support for the Ogg Opus file format since version 1.9
#define TAGLIB_HAS_OPUSFILE \
    ((TAGLIB_MAJOR_VERSION > 1) || ((TAGLIB_MAJOR_VERSION == 1) && (TAGLIB_MINOR_VERSION >= 9)))

// TagLib has support for hasID3v2Tag()/ID3v2Tag() for WAV files since version 1.9
#define TAGLIB_HAS_WAV_ID3V2TAG \
    (TAGLIB_MAJOR_VERSION > 1) || ((TAGLIB_MAJOR_VERSION == 1) && (TAGLIB_MINOR_VERSION >= 9))

// TagLib has support for has<TagType>() style functions since version 1.9
#define TAGLIB_HAS_TAG_CHECK \
    (TAGLIB_MAJOR_VERSION > 1) || ((TAGLIB_MAJOR_VERSION == 1) && (TAGLIB_MINOR_VERSION >= 9))

// TagLib has support for length in milliseconds since version 1.10
#define TAGLIB_HAS_LENGTH_IN_MILLISECONDS \
    (TAGLIB_MAJOR_VERSION > 1) || ((TAGLIB_MAJOR_VERSION == 1) && (TAGLIB_MINOR_VERSION >= 10))

#ifdef _WIN32
static_assert(sizeof(wchar_t) == sizeof(QChar), "wchar_t is not the same size than QChar");
#define TAGLIB_FILENAME_FROM_QSTRING(fileName) (const wchar_t*)fileName.utf16()
// Note: we cannot use QString::toStdWString since QT 4 is compiled with
// '/Zc:wchar_t-' flag and QT 5 not
#else
#define TAGLIB_FILENAME_FROM_QSTRING(fileName) (fileName).toLocal8Bit().constData()
#endif // _WIN32

#include <taglib/tfile.h>
#include <taglib/tmap.h>
#include <taglib/tstringlist.h>

#include <taglib/mpegfile.h>
#include <taglib/mp4file.h>
#include <taglib/flacfile.h>
#include <taglib/vorbisfile.h>
#if TAGLIB_HAS_OPUSFILE
#include <taglib/opusfile.h>
#endif
#include <taglib/wavpackfile.h>
#include <taglib/wavfile.h>
#include <taglib/aifffile.h>

#include <taglib/commentsframe.h>
#include <taglib/textidentificationframe.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacpicture.h>

namespace mixxx {

namespace {

const QString kFileTypeAIFF("aiff");
const QString kFileTypeFLAC("flac");
const QString kFileTypeMP3("mp3");
const QString kFileTypeMP4("mp4");
const QString kFileTypeOggVorbis("ogg");
const QString kFileTypeOggOpus("opus");
const QString kFileTypeWAV("wav");
const QString kFileTypeWavPack("wv");


 bool hasID3v2Tag(TagLib::MPEG::File& file) {
#if TAGLIB_HAS_TAG_CHECK
    return file.hasID3v2Tag();
#else
    return nullptr != file.ID3v2Tag();
#endif
}

 bool hasAPETag(TagLib::MPEG::File& file) {
#if TAGLIB_HAS_TAG_CHECK
    return file.hasAPETag();
#else
    return nullptr != file.APETag();
#endif
}

 bool hasID3v2Tag(TagLib::FLAC::File& file) {
#if TAGLIB_HAS_TAG_CHECK
    return file.hasID3v2Tag();
#else
    return nullptr != file.ID3v2Tag();
#endif
}

 bool hasXiphComment(TagLib::FLAC::File& file) {
#if TAGLIB_HAS_TAG_CHECK
    return file.hasXiphComment();
#else
    return nullptr != file.xiphComment();
#endif
}

 bool hasAPETag(TagLib::WavPack::File& file) {
#if TAGLIB_HAS_TAG_CHECK
    return file.hasAPETag();
#else
    return nullptr != file.APETag();
#endif
}

// Deduce the file type from the file name
QString getFileTypeFromFileName(QString fileName) {
    DEBUG_ASSERT(!fileName.isEmpty());
    const QString fileType(fileName.section(".", -1).toLower().trimmed());
    if ("m4a" == fileType) {
        return kFileTypeMP4;
    }
    if ("aif" == fileType) {
        return kFileTypeAIFF;
    }
    return fileType;
}

// http://id3.org/id3v2.3.0
// "TYER: The 'Year' frame is a numeric string with a year of the
// recording. This frame is always four characters long (until
// the year 10000)."
const QString ID3V2_TYER_FORMAT("yyyy");

// http://id3.org/id3v2.3.0
// "TDAT:  The 'Date' frame is a numeric string in the DDMM
// format containing the date for the recording. This field
// is always four characters long."
const QString ID3V2_TDAT_FORMAT("ddMM");

// Taglib strings can be nullptr and using it could cause some segfaults,
// so in this case it will return a QString()
QString toQString(const TagLib::String& tString) {
    if (tString.isNull()) {
        return QString();
    } else {
        return TStringToQString(tString);
    }
}

// Returns the first element of TagLib string list that is not empty.
QString toQStringFirstNotEmpty(const TagLib::StringList& strList) {
    for (const auto& str: strList) {
        if (!str.isEmpty()) {
            return toQString(str);
        }
    }
    return QString();
}

// Returns the text of an ID3v2 frame as a string.
QString toQString(const TagLib::ID3v2::Frame& frame) {
    return toQString(frame.toString());
}

// Returns the first frame of an ID3v2 tag as a string.
QString toQStringFirstNotEmpty(
        const TagLib::ID3v2::FrameList& frameList) {
    for (const TagLib::ID3v2::Frame* pFrame: frameList) {
        if (nullptr != pFrame) {
            TagLib::String str(pFrame->toString());
            if (!str.isEmpty()) {
                return toQString(str);
            }
        }
    }
    return QString();
}

// Returns the first non-empty value of an MP4 item as a string.
QString toQStringFirstNotEmpty(const TagLib::MP4::Item& mp4Item) {
    return toQStringFirstNotEmpty(mp4Item.toStringList());
}

 TagLib::String toTagLibString(const QString& str) {
    const QByteArray qba(str.toUtf8());
    return TagLib::String(qba.constData(), TagLib::String::UTF8);
}

QString formatBpm(const TrackMetadata& trackMetadata) {
    return Bpm::valueToString(trackMetadata.getBpm().getValue());
}

QString formatBpmInteger(const TrackMetadata& trackMetadata) {
    return QString::number(Bpm::valueToInteger(trackMetadata.getBpm().getValue()));
}

bool parseBpm(TrackMetadata* pTrackMetadata, QString sBpm) {
    DEBUG_ASSERT(pTrackMetadata);

    bool isBpmValid = false;
    const double bpmValue = Bpm::valueFromString(sBpm, &isBpmValid);
    if (isBpmValid) {
        pTrackMetadata->setBpm(Bpm(bpmValue));
    }
    return isBpmValid;
}

QString formatTrackGain(const TrackMetadata& trackMetadata) {
    const double trackGainRatio(trackMetadata.getReplayGain().getRatio());
    return ReplayGain::ratioToString(trackGainRatio);
}

bool parseTrackGain(
        TrackMetadata* pTrackMetadata,
        const QString& dbGain) {
    DEBUG_ASSERT(pTrackMetadata);

    bool isRatioValid = false;
    double ratio = ReplayGain::ratioFromString(dbGain, &isRatioValid);
    if (isRatioValid) {
        // Some applications (e.g. Rapid Evolution 3) write a replay gain
        // of 0 dB even if the replay gain is undefined. To be safe we
        // ignore this special value and instead prefer to recalculate
        // the replay gain.
        if (ratio == ReplayGain::kRatio0dB) {
            // special case
            qDebug() << "Ignoring possibly undefined gain:" << dbGain;
            ratio = ReplayGain::kRatioUndefined;
        }
        ReplayGain replayGain(pTrackMetadata->getReplayGain());
        replayGain.setRatio(ratio);
        pTrackMetadata->setReplayGain(replayGain);
    }
    return isRatioValid;
}

QString formatTrackPeak(const TrackMetadata& trackMetadata) {
    const CSAMPLE trackGainPeak(trackMetadata.getReplayGain().getPeak());
    return ReplayGain::peakToString(trackGainPeak);
}

bool parseTrackPeak(
        TrackMetadata* pTrackMetadata,
        const QString& strPeak) {
    DEBUG_ASSERT(pTrackMetadata);

    bool isPeakValid = false;
    const CSAMPLE peak = ReplayGain::peakFromString(strPeak, &isPeakValid);
    if (isPeakValid) {
        ReplayGain replayGain(pTrackMetadata->getReplayGain());
        replayGain.setPeak(peak);
        pTrackMetadata->setReplayGain(replayGain);
    }
    return isPeakValid;
}

void readAudioProperties(TrackMetadata* pTrackMetadata,
        const TagLib::AudioProperties& audioProperties) {
    DEBUG_ASSERT(pTrackMetadata);

    // NOTE(uklotzde): All audio properties will be updated
    // with the actual (and more precise) values when reading
    // the audio data for this track. Often those properties
    // stored in tags don't match with the corresponding
    // audio data in the file.
    pTrackMetadata->setChannels(audioProperties.channels());
    pTrackMetadata->setSampleRate(audioProperties.sampleRate());
    pTrackMetadata->setBitrate(audioProperties.bitrate());
    const double duration = audioProperties.length();
    pTrackMetadata->setDuration(duration);
}

bool readAudioProperties(TrackMetadata* pTrackMetadata,
        const TagLib::File& file) {
    if (!file.isValid()) {
        return false;
    }
    if (!pTrackMetadata) {
        // implicitly successful
        return true;
    }
    const TagLib::AudioProperties* pAudioProperties =
            file.audioProperties();
    if (!pAudioProperties) {
        qWarning() << "Failed to read audio properties from file"
                << file.name();
        return false;
    }
    readAudioProperties(pTrackMetadata, *pAudioProperties);
    return true;
}

void readTrackMetadataFromTag(TrackMetadata* pTrackMetadata, const TagLib::Tag& tag) {
    if (!pTrackMetadata) {
        return; // nothing to do
    }

    pTrackMetadata->setTitle(toQString(tag.title()));
    pTrackMetadata->setArtist(toQString(tag.artist()));
    pTrackMetadata->setAlbum(toQString(tag.album()));
    pTrackMetadata->setComment(toQString(tag.comment()));
    pTrackMetadata->setGenre(toQString(tag.genre()));

    int iYear = tag.year();
    if (iYear > 0) {
        pTrackMetadata->setYear(QString::number(iYear));
    }

    int iTrack = tag.track();
    if (iTrack > 0) {
        pTrackMetadata->setTrackNumber(QString::number(iTrack));
    }
}

// Workaround for missing const member function in TagLib
 const TagLib::MP4::ItemListMap& getItemListMap(const TagLib::MP4::Tag& tag) {
    return const_cast<TagLib::MP4::Tag&>(tag).itemListMap();
}

void readCoverArtFromID3v2Tag(QImage* pCoverArt, const TagLib::ID3v2::Tag& tag) {
    if (!pCoverArt) {
        return; // nothing to do
    }

    TagLib::ID3v2::FrameList covertArtFrame = tag.frameListMap()["APIC"];
    if (!covertArtFrame.isEmpty()) {
        TagLib::ID3v2::AttachedPictureFrame* picframe =
                static_cast<TagLib::ID3v2::AttachedPictureFrame*>(covertArtFrame.front());
        TagLib::ByteVector data = picframe->picture();
        *pCoverArt = QImage::fromData(
                reinterpret_cast<const uchar *>(data.data()), data.size());
    }
}

void readCoverArtFromAPETag(QImage* pCoverArt, const TagLib::APE::Tag& tag) {
    if (!pCoverArt) {
        return; // nothing to do
    }

    if (tag.itemListMap().contains("COVER ART (FRONT)")) {
        const TagLib::ByteVector nullStringTerminator(1, 0);
        TagLib::ByteVector item =
                tag.itemListMap()["COVER ART (FRONT)"].value();
        int pos = item.find(nullStringTerminator);  // skip the filename
        if (++pos > 0) {
            const TagLib::ByteVector& data = item.mid(pos);
            *pCoverArt = QImage::fromData(
                    reinterpret_cast<const uchar *>(data.data()), data.size());
        }
    }
}

void readCoverArtFromXiphComment(QImage* pCoverArt, const TagLib::Ogg::XiphComment& tag) {
    if (!pCoverArt) {
        return; // nothing to do
    }

    if (tag.fieldListMap().contains("METADATA_BLOCK_PICTURE")) {
        QByteArray data(
                QByteArray::fromBase64(
                        tag.fieldListMap()["METADATA_BLOCK_PICTURE"].front().toCString()));
        TagLib::ByteVector tdata(data.data(), data.size());
        TagLib::FLAC::Picture p(tdata);
        data = QByteArray(p.data().data(), p.data().size());
        *pCoverArt = QImage::fromData(data);
    } else if (tag.fieldListMap().contains("COVERART")) {
        QByteArray data(
                QByteArray::fromBase64(
                        tag.fieldListMap()["COVERART"].toString().toCString()));
        *pCoverArt = QImage::fromData(data);
    }
}

void readCoverArtFromMP4Tag(QImage* pCoverArt, const TagLib::MP4::Tag& tag) {
    if (!pCoverArt) {
        return; // nothing to do
    }

    if (getItemListMap(tag).contains("covr")) {
        TagLib::MP4::CoverArtList coverArtList =
                getItemListMap(tag)["covr"].toCoverArtList();
        TagLib::ByteVector data = coverArtList.front().data();
        *pCoverArt = QImage::fromData(
                reinterpret_cast<const uchar *>(data.data()), data.size());
    }
}

TagLib::String::Type getID3v2StringType(const TagLib::ID3v2::Tag& tag, bool isNumericOrURL = false) {
    TagLib::String::Type stringType;
    // For an overview of the character encodings supported by
    // the different ID3v2 versions please refer to the following
    // resources:
    // http://en.wikipedia.org/wiki/ID3#ID3v2
    // http://id3.org/id3v2.3.0
    // http://id3.org/id3v2.4.0-structure
    if (4 <= tag.header()->majorVersion()) {
        // For ID3v2.4.0 or higher prefer UTF-8, because it is a
        // very compact representation for common cases and it is
        // independent of the byte order.
        stringType = TagLib::String::UTF8;
    } else {
        if (isNumericOrURL) {
            // According to the ID3v2.3.0 specification: "All numeric
            // strings and URLs are always encoded as ISO-8859-1."
            stringType = TagLib::String::Latin1;
        } else {
            // For ID3v2.3.0 use UCS-2 (UTF-16 encoded Unicode with BOM)
            // for arbitrary text, because UTF-8 and UTF-16BE are only
            // supported since ID3v2.4.0 and the alternative ISO-8859-1
            // does not cover all Unicode characters.
            stringType = TagLib::String::UTF16;
        }
    }
    return stringType;
}

// Finds the first comments frame with a matching description.
// If multiple comments frames with matching descriptions exist
// prefer the first with a non-empty content if requested.
TagLib::ID3v2::CommentsFrame* findFirstCommentsFrame(
        const TagLib::ID3v2::Tag& tag,
        const QString& description = QString(),
        bool preferNotEmpty = true) {
    TagLib::ID3v2::FrameList commentsFrames(tag.frameListMap()["COMM"]);
    TagLib::ID3v2::CommentsFrame* pFirstFrame = nullptr;
    for (TagLib::ID3v2::FrameList::ConstIterator it(commentsFrames.begin());
            it != commentsFrames.end(); ++it) {
        auto pFrame =
                dynamic_cast<TagLib::ID3v2::CommentsFrame*>(*it);
        if (nullptr != pFrame) {
            const QString frameDescription(
                    toQString(pFrame->description()));
            if (0 == frameDescription.compare(
                    description, Qt::CaseInsensitive)) {
                if (preferNotEmpty && pFrame->toString().isEmpty()) {
                    // we might need the first matching frame later
                    // even if it is empty
                    if (pFirstFrame == nullptr) {
                        pFirstFrame = pFrame;
                    }
                } else {
                    // found what we are looking for
                    return pFrame;
                }
            }
        }
    }
    // simply return the first matching frame
    return pFirstFrame;
}

// Finds the first text frame that with a matching description.
// If multiple comments frames with matching descriptions exist
// prefer the first with a non-empty content if requested.
TagLib::ID3v2::UserTextIdentificationFrame* findFirstUserTextIdentificationFrame(
        const TagLib::ID3v2::Tag& tag,
        const QString& description,
        bool preferNotEmpty = true) {
    DEBUG_ASSERT(!description.isEmpty());
    TagLib::ID3v2::FrameList textFrames(tag.frameListMap()["TXXX"]);
    TagLib::ID3v2::UserTextIdentificationFrame* pFirstFrame = nullptr;
    for (TagLib::ID3v2::FrameList::ConstIterator it(textFrames.begin());
            it != textFrames.end(); ++it) {
        auto pFrame =
                dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(*it);
        if (nullptr != pFrame) {
            const QString frameDescription(
                    toQString(pFrame->description()));
            if (0 == frameDescription.compare(
                    description, Qt::CaseInsensitive)) {
                if (preferNotEmpty && pFrame->toString().isEmpty()) {
                    // we might need the first matching frame later
                    // even if it is empty
                    if (pFirstFrame == nullptr) {
                        pFirstFrame = pFrame;
                    }
                } else {
                    // found what we are looking for
                    return pFrame;
                }
            }
        }
    }
    // simply return the first matching frame
    return pFirstFrame;
}


bool readMP4Atom(
        const TagLib::MP4::Tag& tag,
        const TagLib::String& key,
        QString* pValue = nullptr) {
    const TagLib::MP4::ItemListMap::ConstIterator it(
            getItemListMap(tag).find(key));
    if (it != getItemListMap(tag).end()) {
        if (nullptr != pValue) {
            *pValue = toQStringFirstNotEmpty((*it).second);
        }
        return true;
    } else {
        return false;
    }
}


bool readAPEItem(
        const TagLib::APE::Tag& tag,
        const TagLib::String& key,
        QString* pValue = nullptr) {
    const TagLib::APE::ItemListMap::ConstIterator it(
            tag.itemListMap().find(key));
    if (it != tag.itemListMap().end() && !(*it).second.values().isEmpty()) {
        if (nullptr != pValue) {
            *pValue = toQStringFirstNotEmpty((*it).second.values());
        }
        return true;
    } else {
        return false;
    }
}

bool readXiphCommentField(
        const TagLib::Ogg::XiphComment& tag,
        const TagLib::String& key,
        QString* pValue = nullptr) {
    const TagLib::Ogg::FieldListMap::ConstIterator it(
            tag.fieldListMap().find(key));
    if (it != tag.fieldListMap().end() && !(*it).second.isEmpty()) {
        if (nullptr != pValue) {
            *pValue = toQStringFirstNotEmpty((*it).second);
        }
        return true;
    } else {
        return false;
    }
}

} // anonymous namespace

void readTrackMetadataFromID3v2Tag(TrackMetadata* pTrackMetadata,
        const TagLib::ID3v2::Tag& tag) {
    if (!pTrackMetadata) {
        return; // nothing to do
    }

    readTrackMetadataFromTag(pTrackMetadata, tag);

    TagLib::ID3v2::CommentsFrame* pCommentsFrame =
            findFirstCommentsFrame(tag);
    if (nullptr != pCommentsFrame) {
        pTrackMetadata->setComment(toQString(*pCommentsFrame));
    }

    const TagLib::ID3v2::FrameList albumArtistFrame(tag.frameListMap()["TPE2"]);
    if (!albumArtistFrame.isEmpty()) {
        pTrackMetadata->setAlbumArtist(toQStringFirstNotEmpty(albumArtistFrame));
    }

    if (pTrackMetadata->getAlbum().isEmpty()) {
        const TagLib::ID3v2::FrameList originalAlbumFrame(
                tag.frameListMap()["TOAL"]);
        pTrackMetadata->setAlbum(toQStringFirstNotEmpty(originalAlbumFrame));
    }

    const TagLib::ID3v2::FrameList composerFrame(tag.frameListMap()["TCOM"]);
    if (!composerFrame.isEmpty()) {
        pTrackMetadata->setComposer(toQStringFirstNotEmpty(composerFrame));
    }

    const TagLib::ID3v2::FrameList groupingFrame(tag.frameListMap()["TIT1"]);
    if (!groupingFrame.isEmpty()) {
        pTrackMetadata->setGrouping(toQStringFirstNotEmpty(groupingFrame));
    }

    // ID3v2.4.0: TDRC replaces TYER + TDAT
    const QString recordingTime(
            toQStringFirstNotEmpty(tag.frameListMap()["TDRC"]));
    if ((4 <= tag.header()->majorVersion()) && !recordingTime.isEmpty()) {
            pTrackMetadata->setYear(recordingTime);
    } else {
        // Fallback to TYER + TDAT
        const QString recordingYear(
                toQStringFirstNotEmpty(tag.frameListMap()["TYER"]).trimmed());
        QString year(recordingYear);
        if (ID3V2_TYER_FORMAT.length() == recordingYear.length()) {
            const QString recordingDate(
                    toQStringFirstNotEmpty(tag.frameListMap()["TDAT"]).trimmed());
            if (ID3V2_TDAT_FORMAT.length() == recordingDate.length()) {
                const QDate date(
                        QDate::fromString(
                                recordingYear + recordingDate,
                                ID3V2_TYER_FORMAT + ID3V2_TDAT_FORMAT));
                if (date.isValid()) {
                    year = TrackMetadata::formatDate(date);
                }
            }
        }
        if (!year.isEmpty()) {
            pTrackMetadata->setYear(year);
        }
    }

    const TagLib::ID3v2::FrameList trackNumberFrame(tag.frameListMap()["TRCK"]);
    if (!trackNumberFrame.isEmpty()) {
        QString trackNumber;
        QString trackTotal;
        TrackNumbers::splitString(
                toQStringFirstNotEmpty(trackNumberFrame),
                &trackNumber,
                &trackTotal);
        pTrackMetadata->setTrackNumber(trackNumber);
        pTrackMetadata->setTrackTotal(trackTotal);
    }

    const TagLib::ID3v2::FrameList bpmFrame(tag.frameListMap()["TBPM"]);
    if (!bpmFrame.isEmpty()) {
        parseBpm(pTrackMetadata, toQStringFirstNotEmpty(bpmFrame));
    }

    const TagLib::ID3v2::FrameList keyFrame(tag.frameListMap()["TKEY"]);
    if (!keyFrame.isEmpty()) {
        pTrackMetadata->setKey(toQStringFirstNotEmpty(keyFrame));
    }

    // Only read track gain (not album gain)
    TagLib::ID3v2::UserTextIdentificationFrame* pTrackGainFrame =
            findFirstUserTextIdentificationFrame(tag, "REPLAYGAIN_TRACK_GAIN");
    if (pTrackGainFrame && (2 <= pTrackGainFrame->fieldList().size())) {
        // The value is stored in the 2nd field
        parseTrackGain(pTrackMetadata,
                toQString(pTrackGainFrame->fieldList()[1]));
    }
    TagLib::ID3v2::UserTextIdentificationFrame* pTrackPeakFrame =
            findFirstUserTextIdentificationFrame(tag, "REPLAYGAIN_TRACK_PEAK");
    if (pTrackPeakFrame && (2 <= pTrackPeakFrame->fieldList().size())) {
        // The value is stored in the 2nd field
        parseTrackPeak(pTrackMetadata,
                toQString(pTrackPeakFrame->fieldList()[1]));
    }
}

void readTrackMetadataFromAPETag(TrackMetadata* pTrackMetadata, const TagLib::APE::Tag& tag) {
    if (!pTrackMetadata) {
        return; // nothing to do
    }

    readTrackMetadataFromTag(pTrackMetadata, tag);

    QString albumArtist;
    if (readAPEItem(tag, "Album Artist", &albumArtist)) {
        pTrackMetadata->setAlbumArtist(albumArtist);
    }

    QString composer;
    if (readAPEItem(tag, "Composer", &composer)) {
        pTrackMetadata->setComposer(composer);
    }

    QString grouping;
    if (readAPEItem(tag, "Grouping", &grouping)) {
        pTrackMetadata->setGrouping(grouping);
    }

    // The release date (ISO 8601 without 'T' separator between date and time)
    // according to the mapping used by MusicBrainz Picard.
    // http://wiki.hydrogenaud.io/index.php?title=APE_date
    // https://picard.musicbrainz.org/docs/mappings
    QString year;
    if (readAPEItem(tag, "Year", &year)) {
        pTrackMetadata->setYear(year);
    }

    QString trackNumber;
    if (readAPEItem(tag, "Track", &trackNumber)) {
        QString trackTotal;
        TrackNumbers::splitString(
                trackNumber,
                &trackNumber,
                &trackTotal);
        pTrackMetadata->setTrackNumber(trackNumber);
        pTrackMetadata->setTrackTotal(trackTotal);
    }

    QString bpm;
    if (readAPEItem(tag, "BPM", &bpm)) {
        parseBpm(pTrackMetadata, bpm);
    }

    // Only read track gain (not album gain)
    QString trackGain;
    if (readAPEItem(tag, "REPLAYGAIN_TRACK_GAIN", &trackGain)) {
        parseTrackGain(pTrackMetadata, trackGain);
    }
    QString trackPeak;
    if (readAPEItem(tag, "REPLAYGAIN_TRACK_PEAK", &trackPeak)) {
        parseTrackPeak(pTrackMetadata, trackPeak);
    }
}

void readTrackMetadataFromXiphComment(TrackMetadata* pTrackMetadata,
        const TagLib::Ogg::XiphComment& tag) {
    if (!pTrackMetadata) {
        return; // nothing to do
    }

    readTrackMetadataFromTag(pTrackMetadata, tag);

    // Some applications (like puddletag up to version 1.0.5) write
    // "COMMENT" instead "DESCRIPTION".
    // Reference: http://www.xiph.org/vorbis/doc/v-comment.html
    if (!readXiphCommentField(tag, "DESCRIPTION")) { // recommended field (already read by TagLib)
        QString comment;
        if (readXiphCommentField(tag, "COMMENT", &comment)) { // alternative field
            pTrackMetadata->setComment(comment);
        }
    }

    QString albumArtist;
    if (readXiphCommentField(tag, "ALBUMARTIST", &albumArtist) || // recommended field
            readXiphCommentField(tag, "ALBUM_ARTIST", &albumArtist) || // alternative field (with underscore character)
            readXiphCommentField(tag, "ALBUM ARTIST", &albumArtist) || // alternative field (with space character)
            readXiphCommentField(tag, "ENSEMBLE", &albumArtist)) { // alternative field
        pTrackMetadata->setAlbumArtist(albumArtist);
    }

    QString composer;
    if (readXiphCommentField(tag, "COMPOSER", &composer)) {
        pTrackMetadata->setComposer(composer);
    }

    QString grouping;
    if (readXiphCommentField(tag, "GROUPING", &grouping)) {
        pTrackMetadata->setGrouping(grouping);
    }

    QString trackNumber;
    if (readXiphCommentField(tag, "TRACKNUMBER", &trackNumber)) {
        QString trackTotal;
        // Split the string, because some applications might decide
        // to store "<trackNumber>/<trackTotal>" in "TRACKNUMBER"
        // even if this is not recommended.
        TrackNumbers::splitString(
                trackNumber,
                &trackNumber,
                &trackTotal);
        if (!readXiphCommentField(tag, "TRACKTOTAL", &trackTotal)) { // recommended field
            (void)readXiphCommentField(tag, "TOTALTRACKS", &trackTotal); // alternative field
        }
        pTrackMetadata->setTrackNumber(trackNumber);
        pTrackMetadata->setTrackTotal(trackTotal);
    }

    // The release date formatted according to ISO 8601. Might
    // be followed by a space character and arbitrary text.
    // http://age.hobba.nl/audio/mirroredpages/ogg-tagging.html
    QString date;
    if (readXiphCommentField(tag, "DATE", &date)) {
        pTrackMetadata->setYear(date);
    }

    QString bpm;
    if (readXiphCommentField(tag, "TEMPO", &bpm) || // recommended field
            readXiphCommentField(tag, "BPM", &bpm)) { // alternative field
        parseBpm(pTrackMetadata, bpm);
    }

    // Only read track gain (not album gain)
    QString trackGain;
    if (readXiphCommentField(tag, "REPLAYGAIN_TRACK_GAIN", &trackGain)) {
        parseTrackGain(pTrackMetadata, trackGain);
    }
    QString trackPeak;
    if (readXiphCommentField(tag, "REPLAYGAIN_TRACK_PEAK", &trackPeak)) {
        parseTrackPeak(pTrackMetadata, trackPeak);
    }

    // Reading key code information
    // Unlike, ID3 tags, there's no standard or recommendation on how to store 'key' code
    //
    // Luckily, there are only a few tools for that, e.g., Rapid Evolution (RE).
    // Assuming no distinction between start and end key, RE uses a "INITIALKEY"
    // or a "KEY" vorbis comment.
    QString key;
    if (readXiphCommentField(tag, "INITIALKEY", &key) || // recommended field
            readXiphCommentField(tag, "KEY", &key)) { // alternative field
        pTrackMetadata->setKey(key);
    }
}

void readTrackMetadataFromMP4Tag(TrackMetadata* pTrackMetadata, const TagLib::MP4::Tag& tag) {
    if (!pTrackMetadata) {
        return; // nothing to do
    }

    readTrackMetadataFromTag(pTrackMetadata, tag);

    QString albumArtist;
    if (readMP4Atom(tag, "aART", &albumArtist)) {
        pTrackMetadata->setAlbumArtist(albumArtist);
    }

    QString composer;
    if (readMP4Atom(tag, "\251wrt", &composer)) {
        pTrackMetadata->setComposer(composer);
    }

    QString grouping;
    if (readMP4Atom(tag, "\251grp", &grouping)) {
        pTrackMetadata->setGrouping(grouping);
    }

    QString year;
    if (readMP4Atom(tag, "\251day", &year)) {
        pTrackMetadata->setYear(year);
    }

    // Read track number/total pair
    if (getItemListMap(tag).contains("trkn")) {
        const TagLib::MP4::Item::IntPair trknPair(getItemListMap(tag)["trkn"].toIntPair());
        const TrackNumbers trackNumbers(trknPair.first, trknPair.second);
        QString trackNumber;
        QString trackTotal;
        trackNumbers.toStrings(&trackNumber, &trackTotal);
        pTrackMetadata->setTrackNumber(trackNumber);
        pTrackMetadata->setTrackTotal(trackTotal);
    }

    QString bpm;
    if (readMP4Atom(tag, "----:com.apple.iTunes:BPM", &bpm)) {
        // This is the preferred field for storing the BPM
        // with fractional digits as a floating-point value.
        // If this field contains a valid value the integer
        // BPM value that might have been read before is
        // overwritten.
        parseBpm(pTrackMetadata, bpm);
    } else if (getItemListMap(tag).contains("tmpo")) {
            // Read the BPM as an integer value.
            const TagLib::MP4::Item& item = getItemListMap(tag)["tmpo"];
#if TAGLIB_HAS_MP4_ATOM_TYPES
            if (item.atomDataType() == TagLib::MP4::TypeInteger) {
                pTrackMetadata->setBpm(Bpm(item.toInt()));
            }
#else
            pTrackMetadata->setBpm(Bpm(item.toInt()));
#endif
    }

    // Only read track gain (not album gain)
    QString trackGain;
    if (readMP4Atom(tag, "----:com.apple.iTunes:replaygain_track_gain", &trackGain)) {
        parseTrackGain(pTrackMetadata, trackGain);
    }
    QString trackPeak;
    if (readMP4Atom(tag, "----:com.apple.iTunes:replaygain_track_peak", &trackPeak)) {
        parseTrackPeak(pTrackMetadata, trackPeak);
    }

    QString key;
    if (readMP4Atom(tag, "----:com.apple.iTunes:initialkey", &key) || // preferred (conforms to MixedInKey, Serato, Traktor)
            readMP4Atom(tag, "----:com.apple.iTunes:KEY", &key)) { // alternative (conforms to Rapid Evolution)
        pTrackMetadata->setKey(key);
    }
}


bool readTrackMetadataAndCoverArtFromFile(TrackMetadata* pTrackMetadata, QImage* pCoverArt, QString fileName) {
    const QString fileType(getFileTypeFromFileName(fileName));

    qDebug() << "Reading tags from file" << fileName << "of type" << fileType
            << ":" << (pTrackMetadata ? "parsing" : "ignoring") << "track metadata"
            << "," << (pCoverArt ? "parsing" : "ignoring") << "cover art";

    // Rationale: If a file contains different types of tags only
    // a single type of tag will be read. Tag types are read in a
    // fixed order. Both track metadata and cover art will be read
    // from the same tag types. Only the first available tag type
    // is read and data in subsequent tags is ignored.

    if (kFileTypeMP3 == fileType) {
        TagLib::MPEG::File file(TAGLIB_FILENAME_FROM_QSTRING(fileName));
        if (readAudioProperties(pTrackMetadata, file)) {
            const TagLib::ID3v2::Tag* pID3v2Tag =
                    hasID3v2Tag(file) ? file.ID3v2Tag() : nullptr;
            if (pID3v2Tag) {
                readTrackMetadataFromID3v2Tag(pTrackMetadata, *pID3v2Tag);
                readCoverArtFromID3v2Tag(pCoverArt, *pID3v2Tag);
                return true;
            } else {
                const TagLib::APE::Tag* pAPETag =
                        hasAPETag(file) ? file.APETag() : nullptr;
                if (pAPETag) {
                    readTrackMetadataFromAPETag(pTrackMetadata, *pAPETag);
                    readCoverArtFromAPETag(pCoverArt, *pAPETag);
                    return true;
                } else {
                    // fallback
                    const TagLib::Tag* pTag(file.tag());
                    if (pTag) {
                        readTrackMetadataFromTag(pTrackMetadata, *pTag);
                        return true;
                    }
                }
            }
        }
    } else if (kFileTypeMP4 == fileType) {
        TagLib::MP4::File file(TAGLIB_FILENAME_FROM_QSTRING(fileName));
        if (readAudioProperties(pTrackMetadata, file)) {
            const TagLib::MP4::Tag* pMP4Tag = file.tag();
            if (pMP4Tag) {
                readTrackMetadataFromMP4Tag(pTrackMetadata, *pMP4Tag);
                readCoverArtFromMP4Tag(pCoverArt, *pMP4Tag);
                return true;
            } else {
                // fallback
                const TagLib::Tag* pTag(file.tag());
                if (pTag) {
                    readTrackMetadataFromTag(pTrackMetadata, *pTag);
                    return true;
                }
            }
        }
    } else if (kFileTypeFLAC == fileType) {
        TagLib::FLAC::File file(TAGLIB_FILENAME_FROM_QSTRING(fileName));
        if (pCoverArt) {
            // FLAC files may contain cover art that is not part
            // of any tag. Use the first picture from this list
            // as the default picture for the cover art.
            TagLib::List<TagLib::FLAC::Picture*> covers = file.pictureList();
            if (!covers.isEmpty()) {
                std::list<TagLib::FLAC::Picture*>::iterator it = covers.begin();
                TagLib::FLAC::Picture* cover = *it;
                *pCoverArt = QImage::fromData(
                        QByteArray(cover->data().data(), cover->data().size()));
            }
        }
        if (readAudioProperties(pTrackMetadata, file)) {
            const TagLib::Ogg::XiphComment* pXiphComment =
                    hasXiphComment(file) ? file.xiphComment() : nullptr;
            if (pXiphComment) {
                readTrackMetadataFromXiphComment(pTrackMetadata, *pXiphComment);
                readCoverArtFromXiphComment(pCoverArt, *pXiphComment);
                return true;
            } else {
                const TagLib::ID3v2::Tag* pID3v2Tag =
                        hasID3v2Tag(file) ? file.ID3v2Tag() : nullptr;
                if (pID3v2Tag) {
                    readTrackMetadataFromID3v2Tag(pTrackMetadata, *pID3v2Tag);
                    readCoverArtFromID3v2Tag(pCoverArt, *pID3v2Tag);
                    return true;
                } else {
                    // fallback
                    const TagLib::Tag* pTag(file.tag());
                    if (pTag) {
                        readTrackMetadataFromTag(pTrackMetadata, *pTag);
                        return true;
                    }
                }
            }
        }
    } else if (kFileTypeOggVorbis == fileType) {
        TagLib::Ogg::Vorbis::File file(TAGLIB_FILENAME_FROM_QSTRING(fileName));
        if (readAudioProperties(pTrackMetadata, file)) {
            const TagLib::Ogg::XiphComment* pXiphComment = file.tag();
            if (pXiphComment) {
                readTrackMetadataFromXiphComment(pTrackMetadata, *pXiphComment);
                readCoverArtFromXiphComment(pCoverArt, *pXiphComment);
                return true;
            } else {
                // fallback
                const TagLib::Tag* pTag(file.tag());
                if (pTag) {
                    readTrackMetadataFromTag(pTrackMetadata, *pTag);
                    return true;
                }
            }
        }
#if TAGLIB_HAS_OPUSFILE
    } else if (kFileTypeOggOpus == fileType) {
        TagLib::Ogg::Opus::File file(TAGLIB_FILENAME_FROM_QSTRING(fileName));
        if (readAudioProperties(pTrackMetadata, file)) {
            const TagLib::Ogg::XiphComment* pXiphComment = file.tag();
            if (pXiphComment) {
                readTrackMetadataFromXiphComment(pTrackMetadata, *pXiphComment);
                readCoverArtFromXiphComment(pCoverArt, *pXiphComment);
                 return true;
            } else {
                // fallback
                const TagLib::Tag* pTag(file.tag());
                if (pTag) {
                    readTrackMetadataFromTag(pTrackMetadata, *pTag);
                    return true;
                }
            }
        }
#endif // TAGLIB_HAS_OPUSFILE
    } else if (kFileTypeWavPack == fileType) {
        TagLib::WavPack::File file(TAGLIB_FILENAME_FROM_QSTRING(fileName));
        if (readAudioProperties(pTrackMetadata, file)) {
            const TagLib::APE::Tag* pAPETag =
                    hasAPETag(file) ? file.APETag() : nullptr;
            if (pAPETag) {
                readTrackMetadataFromAPETag(pTrackMetadata, *pAPETag);
                readCoverArtFromAPETag(pCoverArt, *pAPETag);
                return true;
            } else {
                // fallback
                const TagLib::Tag* pTag(file.tag());
                if (pTag) {
                    readTrackMetadataFromTag(pTrackMetadata, *pTag);
                    return true;
                }
            }
        }
    } else if (kFileTypeWAV == fileType) {

        TagLib::RIFF::WAV::File file(TAGLIB_FILENAME_FROM_QSTRING(fileName));
        if (readAudioProperties(pTrackMetadata, file)) {
#if TAGLIB_HAS_WAV_ID3V2TAG
            const TagLib::ID3v2::Tag* pID3v2Tag =
                    file.hasID3v2Tag() ? file.ID3v2Tag() : nullptr;
#else
            const TagLib::ID3v2::Tag* pID3v2Tag = file.tag();
#endif
            if (pID3v2Tag) {
                readTrackMetadataFromID3v2Tag(pTrackMetadata, *pID3v2Tag);
                readCoverArtFromID3v2Tag(pCoverArt, *pID3v2Tag);
                return true;
            } else {
                // fallback
                const TagLib::Tag* pTag(file.tag());
                if (pTag) {
                    readTrackMetadataFromTag(pTrackMetadata, *pTag);
                    return true;
                }
            }
        }
    } else if (kFileTypeAIFF == fileType) {
        TagLib::RIFF::AIFF::File file(TAGLIB_FILENAME_FROM_QSTRING(fileName));
        if (readAudioProperties(pTrackMetadata, file)) {
            const TagLib::ID3v2::Tag* pID3v2Tag = file.tag();
            if (pID3v2Tag) {
                readTrackMetadataFromID3v2Tag(pTrackMetadata, *pID3v2Tag);
                readCoverArtFromID3v2Tag(pCoverArt, *pID3v2Tag);
                return true;
            } else {
                // fallback
                const TagLib::Tag* pTag(file.tag());
                if (pTag) {
                    readTrackMetadataFromTag(pTrackMetadata, *pTag);
                    return true;
                }
            }
        }
    } else {
        qWarning() << "Unsupported file type" << fileType;
    }
    qWarning() << "Failed to read track metadata from file" << fileName;
    return false;
}
} //namespace mixxx
