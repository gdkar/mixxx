_Pragma("once")
#include "engine/engineobject.h"
#include "util/assert.h"
#include "util/sample.h"
#include <memory>
#include <utility>

class EngineFilterDelay : public EngineObjectConstIn {
    Q_OBJECT;
  protected:
    int m_delaySamples;
    int m_oldDelaySamples;
    int m_delayPos;
    bool m_doRamping;
    bool m_doStart;
    size_t m_size{0};
    size_t m_mask{0};
    std::unique_ptr<CSAMPLE[]> m_buf;
  public:
    EngineFilterDelay(QObject *pParent, size_t size);
    virtual ~EngineFilterDelay();
    void pauseFilter() ;
    void setDelay(unsigned int delaySamples);
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize);
};
