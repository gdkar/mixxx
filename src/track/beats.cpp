#include "track/beats.h"

BeatIterator::~BeatIterator() = default;
bool BeatIterator::hasNext() const
{
  return false;
}
double BeatIterator::next()
{
  return -1;
}
Beats::Beats(QObject *p)
  : QObject(p)
{
}
Beats::~Beats() = default;
