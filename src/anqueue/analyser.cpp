#include "anqueue/analyser.h"

Analyser::Analyser(ConfigObject<ConfigValue>*c,QObject *p)
  : QObject(p)
  , m_pConfig(c)
{
}
Analyser::~Analyser() = default;
