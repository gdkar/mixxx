#include "metadata/trackmetadata.h"

#include "util/math.h"

namespace Mixxx {

/*static*/ const double TrackMetadata::kBpmUndefined = 0.0;
/*static*/ const double TrackMetadata::kBpmMin = 0.0; // lower bound (exclusive)
/*static*/ const double TrackMetadata::kBpmMax = 300.0; // upper bound (inclusive)

/*static*/ const double TrackMetadata::kReplayGainUndefined = 0.0;
/*static*/ const double TrackMetadata::kReplayGainMin = 0.0; // lower bound (inclusive)
/*static*/ const double TrackMetadata::kReplayGain0dB = 1.0;

/*static*/ const int TrackMetadata::kCalendarYearInvalid = 0;

double TrackMetadata::parseBpm(const QString& sBpm, bool* pValid) {
    if (pValid) *pValid = false;
    if (sBpm.trimmed().isEmpty()) return kBpmUndefined;
    auto bpmValid = false;
    auto bpm = sBpm.toDouble(&bpmValid);
    if (bpmValid)
    {
        if (kBpmUndefined == bpm)
        {
            // special case
            if (pValid) *pValid = true;
            return bpm;
        }
        while (kBpmMax < bpm)
        {
            // Some applications might store the BPM as an
            // integer scaled by a factor of 10 or 100 to
            // preserve fractional digits.
            qDebug() << "Scaling BPM value:" << bpm;
            bpm /= 10.0;
        }
        if (TrackMetadata::isBpmValid(bpm))
        {
            if (pValid) *pValid = true;
            return bpm;
        } else qDebug() << "Invalid BPM value:" << sBpm << "->" << bpm;
    } else qDebug() << "Failed to parse BPM:" << sBpm;
    return kBpmUndefined;
}

QString TrackMetadata::formatBpm(double bpm)
{
    if (TrackMetadata::isBpmValid(bpm)) return QString::number(bpm);
    else return QString();
}
QString TrackMetadata::formatBpm(int bpm)
{
    if (TrackMetadata::isBpmValid(bpm)) return QString::number(bpm);
    else return QString();
}

namespace {

const QString kReplayGainUnit("dB");
const QString kReplayGainSuffix(" " + kReplayGainUnit);

} // anonymous namespace
TrackMetadata::~TrackMetadata() = default;
QString TrackMetadata::getArtist() const
{
  return m_artist;
}
void TrackMetadata::setArtist(QString s)
{
  if ( m_artist != s )
  {
    m_artist = s;
    emit updated();
  }
}
QString TrackMetadata::getTitle() const
{
  return m_title;
}
void TrackMetadata::setTitle(QString s)
{
  if ( m_title != s )
  {
    m_title = s;
    emit updated();
  }
}
QString TrackMetadata::getAlbum() const
{
  return m_album;
}
void TrackMetadata::setAlbum(QString s)
{
  if ( m_album != s)
  {
    m_album = s;
    emit updated();
  }
}
QString TrackMetadata::getAlbumArtist() const
{
  return m_albumArtist;
}
void TrackMetadata::setAlbumArtist(QString s)
{
  if ( m_albumArtist != s )
  {
    m_albumArtist = s;
    emit updated();
  }
}
QString TrackMetadata::getGenre() const
{
  return m_genre;
}
void TrackMetadata::setGenre(QString s)
{
  if ( m_genre != s )
  {
    m_genre = s;
    emit updated();
  }
}
QString TrackMetadata::getComment() const
{
  return m_comment;
}
void TrackMetadata::setComment(QString s)
{
  if ( m_comment != s )
  {
    m_comment = s;
    emit updated();
  }
}
QString TrackMetadata::getYear() const
{
  return m_year;
}
void TrackMetadata::setYear(QString s)
{
  if ( m_year != s )
  {
    m_year = s;
    emit updated();
  }
}
QString TrackMetadata::getTrackNumber() const
{
  return m_trackNumber;
}
void TrackMetadata::setTrackNumber(QString s)
{
  if ( m_trackNumber != s )
  {
    m_trackNumber = s;
    emit updated();
  }
}
QString TrackMetadata::getComposer() const
{
  return m_composer;
}
void TrackMetadata::setComposer(QString s)
{
  if ( m_composer != s )
  {
    m_composer = s;
    emit updated();
  }
}
QString TrackMetadata::getGrouping() const
{
  return m_grouping;
}
void TrackMetadata::setGrouping(QString s)
{
  if ( m_grouping != s)
  {
    m_grouping = s;
    emit updated();
  }
}
QString TrackMetadata::getKey() const
{
  return m_key;
}
void TrackMetadata::setKey(QString s)
{
  if ( m_key != s )
  {
    m_key = s;
    emit updated();
  }
}
int TrackMetadata::getChannels() const
{
  return m_channels;
}
void TrackMetadata::setChannels(int i)
{
  if ( m_channels != i )
  {
    m_channels = i;
    emit updated();
  }
}
int TrackMetadata::getSampleRate() const
{
  return m_sampleRate;
}
void TrackMetadata::setSampleRate(int i)
{
  if ( m_sampleRate != i )
  {
    m_sampleRate = i;
    emit updated();
  }
}
int TrackMetadata::getBitrate() const
{
  return m_bitrate;
}
void TrackMetadata::setBitrate(int i)
{
  if ( m_bitrate != i )
  {
    m_bitrate = i;
    emit updated();
  }
}
int TrackMetadata::getDuration() const
{
  return m_duration;
}
void TrackMetadata::setDuration(int i)
{
  if ( m_duration != i )
  {
    m_duration = i;
    emit updated();
  }
}
double TrackMetadata::getBpm() const
{
  return m_bpm;
}
int TrackMetadata::getBpmAsInteger() const
{
  return std::round(getBpm());
}
/* static */ bool TrackMetadata::isBpmValid(double d)
{
  return (kBpmMin < d ) && (kBpmMax >= d);
}
bool TrackMetadata::isBpmValid() const
{
  return isBpmValid(getBpm());
}
void TrackMetadata::setBpm(double d)
{
  if ( m_bpm != d )
  {
    m_bpm = d;
    updated();
  }
}
void TrackMetadata::resetBpm()
{
  setBpm(kBpmUndefined);
}
double TrackMetadata::getReplayGain() const
{
  return m_replayGain;
}
/* static */ bool TrackMetadata::isReplayGainValid(double d)
{
  return kReplayGainMin < d;
}
bool TrackMetadata::isReplayGainValid() const
{
  return isReplayGainValid(getReplayGain());
}
void TrackMetadata::setReplayGain(double d)
{
  if ( m_replayGain != d )
  {
    m_replayGain = d;
    emit updated();
  }
}
void TrackMetadata::resetReplayGain()
{
  setReplayGain(kReplayGainUndefined);
}
/* static */ QDate TrackMetadata::parseDate(QString s)
{
  return QDate::fromString(s.trimmed().replace(" ",""),Qt::ISODate);
}
/* static */ QDateTime TrackMetadata::parseDateTime(QString s)
{
  return QDateTime::fromString(s.trimmed().replace(" ",""),Qt::ISODate);
}
/* static */ QString TrackMetadata::formatDate(QDate d)
{
  return d.toString(Qt::ISODate);
}
/* static */ QString TrackMetadata::formatDateTime(QDateTime d)
{
  return d.toString(Qt::ISODate);
}
double TrackMetadata::parseReplayGain(QString sReplayGain, bool* pValid)
{
    if (pValid) *pValid = false;
    auto  normalizedReplayGain = sReplayGain.trimmed();
    auto plusIndex = normalizedReplayGain.indexOf('+');
    if (!plusIndex) normalizedReplayGain = normalizedReplayGain.mid(plusIndex + 1).trimmed();
    auto unitIndex = normalizedReplayGain.lastIndexOf(kReplayGainUnit, -1, Qt::CaseInsensitive);
    if ((0 <= unitIndex) && ((normalizedReplayGain.length() - 2) == unitIndex))
    {
        // strip trailing unit suffix
        normalizedReplayGain = normalizedReplayGain.left(unitIndex).trimmed();
    }
    if (normalizedReplayGain.isEmpty()) return kReplayGainUndefined;
    auto replayGainValid = false;
    auto replayGainDb = normalizedReplayGain.toDouble(&replayGainValid);
    if (replayGainValid)
    {
        auto replayGain = db2ratio(replayGainDb);
        DEBUG_ASSERT(kReplayGainUndefined != replayGain);
        // Some applications (e.g. Rapid Evolution 3) write a replay gain
        // of 0 dB even if the replay gain is undefined. To be safe we
        // ignore this special value and instead prefer to recalculate
        // the replay gain.
        if (kReplayGain0dB == replayGain)
        {
            // special case
            qDebug() << "Ignoring possibly undefined replay gain:" << formatReplayGain(replayGain);
            if (pValid) *pValid = true;
            return kReplayGainUndefined;
        }
        if (TrackMetadata::isReplayGainValid(replayGain))
        {
            if (pValid) *pValid = true;
            return replayGain;
        } else qDebug() << "Invalid replay gain value:" << sReplayGain << " -> "<< replayGain;
    } else qDebug() << "Failed to parse replay gain:" << sReplayGain;
    return kReplayGainUndefined;
}
QString TrackMetadata::formatReplayGain(double replayGain)
{
    if (isReplayGainValid(replayGain))
        return QString::number(ratio2db(replayGain)) + kReplayGainSuffix;
    else return QString();
}
int TrackMetadata::parseCalendarYear(QString year, bool* pValid)
{
    auto dateTime = QDateTime(parseDateTime(year));
    if (0 < dateTime.date().year())
    {
        if (pValid) *pValid = true;
        return dateTime.date().year();
    }
    else
    {
        auto calendarYearValid = false;
        // Ignore everything beginning with the first dash '-'
        // to successfully parse the calendar year of incomplete
        // dates like yyyy-MM or 2015-W07.
        auto calendarYearSection(year.section('-', 0, 0).trimmed());
        auto calendarYear = calendarYearSection.toInt(&calendarYearValid);
        if (calendarYearValid) calendarYearValid = 0 < calendarYear;
        if (pValid) *pValid = calendarYearValid;
        if (calendarYearValid) return calendarYear;
        else                   return kCalendarYearInvalid;
    }
}
QString TrackMetadata::formatCalendarYear(QString year, bool* pValid)
{
    auto calendarYearValid = false;
    auto calendarYear = parseCalendarYear(year, &calendarYearValid);
    if (pValid) *pValid = calendarYearValid;
    if (calendarYearValid) return QString::number(calendarYear);
    else return QString(); // empty string
}
QString TrackMetadata::reformatYear(QString year)
{
    auto dateTime = QDateTime(parseDateTime(year));
    if (dateTime.isValid()) return formatDateTime(dateTime);
    const QDate date(dateTime.date());
    if (date.isValid()) return formatDate(date);
    auto calendarYearValid = false;
    auto calendarYear = formatCalendarYear(year, &calendarYearValid);
    if (calendarYearValid) return calendarYear;
    // just trim and simplify whitespaces
    return year.simplified();
}

TrackMetadata::TrackMetadata() :
        m_channels(0),
        m_sampleRate(0),
        m_bitrate(0),
        m_duration(0),
        m_bpm(kBpmUndefined),
        m_replayGain(kReplayGainUndefined)
{
}
} //namespace Mixxx
