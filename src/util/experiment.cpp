#include "util/experiment.h"

// static
std::atomic<Experiment::Mode> Experiment::s_mode{Experiment::Mode::Off};
bool Experiment::isEnabled()
{
  return s_mode.load() != Mode::Off;
}
void Experiment::disable()
{
  qDebug() << "Experiment::setExperiment Off";
  s_mode.store(Mode::Off);
}
void Experiment::setExperiment()
{
  qDebug() << "Experiment::setExperient Experiment";
  s_mode.store(Mode::Experiment);
}
void Experiment::setBase()
{
  qDebug() << "Experiment::setExperient Experiment";
  s_mode.store(Mode::Base);
}
bool Experiment::isExperiment()
{
  return s_mode.load() == Mode::Experiment;
}
bool Experiment::isBase()
{
  return s_mode.load() == Mode::Base;
}
Experiment::Mode Experiment::mode()
{
  return s_mode.load();
}
