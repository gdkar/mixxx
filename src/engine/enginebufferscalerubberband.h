_Pragma("once")
#include "engine/enginebufferscale.h"

namespace RubberBand {
class RubberBandStretcher;
}  // namespace RubberBand

class ReadAheadManager;

// Uses librubberband to scale audio.  This class is not thread safe.
class EngineBufferScaleRubberBand : public EngineBufferScale {
    Q_OBJECT
  public:
    EngineBufferScaleRubberBand(ReadAheadManager* pRAMan,QObject *pParent);
    virtual ~EngineBufferScaleRubberBand();

    virtual void setScaleParameters(double base_rate,
                                    double* pTempoRatio,
                                    double* pPitchRatio);

    virtual void setSampleRate(int iSampleRate);

    // Read and scale buf_size samples from the provided RAMAN.
    virtual CSAMPLE* getScaled(unsigned long buf_size);

    // Flush buffer.
    virtual void clear();

    // Reset RubberBand library with new samplerate.
    void initializeRubberBand(int iSampleRate);
  private:
    void deinterleaveAndProcess(const CSAMPLE* pBuffer, size_t frames, bool flush);
    size_t retrieveAndDeinterleave(CSAMPLE* pBuffer, size_t frames);

    // Holds the playback direction
    bool m_bBackwards = false;
    std::unique_ptr<CSAMPLE[]> m_retrieve_buffer[2];
    std::unique_ptr<CSAMPLE[]> m_buffer_back;
    std::unique_ptr<RubberBand::RubberBandStretcher> m_pRubberBand;
};
