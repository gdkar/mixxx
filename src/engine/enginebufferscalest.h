#ifndef ENGINEBUFFERSCALEST_H
#define ENGINEBUFFERSCALEST_H

#include "engine/enginebufferscale.h"
#include "util/memory.h"

class ReadAheadManager;

namespace soundtouch {
class SoundTouch;
}  // namespace soundtouch

// Uses libsoundtouch to scale audio.
class EngineBufferScaleST : public EngineBufferScale {
    Q_OBJECT
  public:
    EngineBufferScaleST(ReadAheadManager* pReadAheadManager, QObject *pParent );
   ~EngineBufferScaleST() override;
    void setScaleParameters(double base_rate,
                            double* pTempoRatio,
                            double* pPitchRatio) override;

    void setSampleRate(SINT iSampleRate) override;
    // Scale buffer.
    double scaleBuffer(
            CSAMPLE* pOutputBuffer,
            SINT iOutputBufferSize) override;
    // Flush buffer.
    void clear() override;
  private:
    // Temporary buffer for reading from the RAMAN.
    SINT buffer_back_size;
    std::unique_ptr<CSAMPLE[]> buffer_back;
    // SoundTouch time/pitch scaling lib
    std::unique_ptr<soundtouch::SoundTouch> m_pSoundTouch;
    // Holds the playback direction.
    bool m_bBackwards;
};

#endif
