#include "track/beats.h"

BeatIterator::BeatIterator() = default;
BeatIterator::BeatIterator(const Beats *pBeats, int index, double stop)
  : m_pBeats(pBeats)
  , m_index (index)
  , m_stop  (stop)
{
}
BeatIterator::~BeatIterator() = default;
bool BeatIterator::hasNext() const
{
  if ( !m_pBeats || m_stop < 0 ) return false;
  auto d = m_pBeats->beatAtIndex(m_index);
  if ( d < 0 || d >= m_stop ) return false;
  return true;
}
double BeatIterator::next()
{
  if ( !m_pBeats || m_stop < 0 ) return -1;
  auto d = m_pBeats->beatAtIndex(m_index);
  if ( d < 0 || d >= m_stop ) return false;
  m_index ++;
  return d;
}
Beats::Beats(QObject *p)
  : QObject(p)
{
}
Beats::~Beats() = default;

BeatIterator Beat::findBeats(double startSample, double stopSample) const
{
    QMutexLocker locker(&m_mutex);
    startSample = std::max(0,startSample);
    if (!isValid() || startSample > stopSample) return BeatIterator{};
    return BeatIterator(this,findIndexNear(startSample),stopSample);
}
