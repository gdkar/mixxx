#ifndef SIDECHAINWORKER_H
#define SIDECHAINWORKER_H

#include "util/types.h"

class SideChainWorker {
  public:
    SideChainWorker()          = default;
    virtual ~SideChainWorker() = default;
    virtual void process(const CSAMPLE* pBuffer, int iBufferSize) = 0;
    virtual void shutdown() = 0;
};

#endif /* SIDECHAINWORKER_H */
