#include "track/beats.h"

BeatIterator::BeatIterator() = default;
BeatIterator::BeatIterator(Beats *pBeats, double curr, double stop)
  : m_pBeats(pBeats)
  , m_stop  (stop)
  , m_current(curr)
{
}
BeatIterator::~BeatIterator() = default;
bool BeatIterator::hasNext() const
{
  return m_stop >= 0 && m_current < m_stop;
}
double BeatIterator::next()
{
  auto ret = m_current;
  if(ret >= 0 && ret < m_stop) m_current = m_pBeats->findNextBeat(ret);
  return ret;
}
Beats::Beats(QObject *p)
  : QObject(p)
{
}
Beats::~Beats() = default;
