_Pragma("once")
#include <cstring>
#include <memory>
#include <algorithm>
#include "engine/engineobject.h"
#include "util/assert.h"

class EngineFilterDelay : public EngineObjectConstIn
{
  Q_OBJECT
  public:
    EngineFilterDelay(int SIZE, QObject *pParent);
    virtual ~EngineFilterDelay() ;
    void pauseFilter();
    void setDelay(unsigned int delaySamples);
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput, int iBufferSize);
  protected:
    int m_delaySamples     = 0;
    int m_oldDelaySamples  = 0;
    int m_delayPos         = 0;
    int m_size;
    std::unique_ptr<CSAMPLE[]> m_buf;
    bool m_doRamping       = false;
    bool m_doStart         = false;
};

