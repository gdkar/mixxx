#ifndef ENGINEBUFFERSCALELINEAR_H
#define ENGINEBUFFERSCALELINEAR_H

#include "engine/enginebufferscale.h"
#include "engine/readaheadmanager.h"

/** Number of samples to read ahead */
static constexpr const int kiLinearScaleReadAheadLength = 10240;


class EngineBufferScaleLinear : public EngineBufferScale  {
    Q_OBJECT
  public:
    EngineBufferScaleLinear(ReadAheadManager *pReadAheadManager, QObject *pParent);
   ~EngineBufferScaleLinear() override;

    double getScaled(CSAMPLE* pOutput, int iBufferSize) override;
    EngineBufferScaleLinear(ReadAheadManager *pReadAheadManager, QObject *pParent);
    ~EngineBufferScaleLinear() override;

    double scaleBuffer(
            CSAMPLE* pOutputBuffer,
            SINT iOutputBufferSize) override;
    void clear() override;

    void setScaleParameters(double base_rate,
                            double* pTempoRatio,
                             double* pPitchRatio) override;

  private:
    SINT do_scale(CSAMPLE* buf, SINT buf_size);
    SINT do_copy(CSAMPLE* buf, SINT buf_size);

    // The read-ahead manager that we use to fetch samples
    ReadAheadManager* m_pReadAheadManager;

    // Buffer for handling calls to ReadAheadManager
    CSAMPLE* m_bufferInt;
    SINT m_bufferIntSize;

    CSAMPLE m_floorSampleOld[2];

    bool m_bClear;
    double m_dRate;
    double m_dOldRate;

    double m_dCurrentFrame;
    double m_dNextFrame;
};

#endif
