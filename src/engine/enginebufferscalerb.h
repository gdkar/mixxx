#ifndef ENGINEBUFFERSCALERUBBERBAND_H
#define ENGINEBUFFERSCALERUBBERBAND_H

#include "engine/enginebufferscale.h"

namespace RubberBand {
class RubberBandStretcher;
}  // namespace RubberBand

class ReadAheadManager;

// Uses librubberband to scale audio.  This class is not thread safe.
class EngineBufferScaleRB : public EngineBufferScale {
    Q_OBJECT
  public:
    EngineBufferScaleRB(ReadAheadManager* pReadAheadManager, QObject *pParent );
    ~EngineBufferScaleRB() override;

    void setScaleParameters(double base_rate,
                            double* pTempoRatio,
                            double* pPitchRatio) override;

    void setSampleRate(int iSampleRate) override;

    // Read and scale buf_size samples from the provided RAMAN.
    double getScaled(CSAMPLE* pOutput, int iBufferSize) override;

    // Flush buffer.
    void clear() override;

    // Reset RubberBand library with new samplerate.
    void initializeRubberBand(int iSampleRate);
  private:
    void deinterleaveAndProcess(const CSAMPLE* pBuffer, size_t frames, bool flush);
    size_t retrieveAndDeinterleave(CSAMPLE* pBuffer, size_t frames);

    // Holds the playback direction
    bool m_bBackwards;

    CSAMPLE* m_retrieve_buffer[2];
    CSAMPLE* m_buffer_back;

    RubberBand::RubberBandStretcher* m_pRubberBand;
};


#endif /* ENGINEBUFFERSCALERUBBERBAND_H */
