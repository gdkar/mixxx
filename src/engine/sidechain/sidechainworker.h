_Pragma("once")
#include "util/types.h"

class SideChainWorker {
  public:
    SideChainWorker() = default;
    virtual ~SideChainWorker() = default;
    virtual void process(const CSAMPLE* pBuffer, int iBufferSize) = 0;
    virtual void shutdown() = 0;
};
