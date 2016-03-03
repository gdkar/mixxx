/***************************************************************************
                          enginebufferscalelinear.h  -  description
                             -------------------
    begin                : Mon Apr 14 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

_Pragma("once")
#include "engine/enginebufferscale.h"
#include "engine/readaheadmanager.h"

/**
  *@author Tue & Ken Haste Andersen
  */


/** Number of samples to read ahead */
const int kiLinearScaleReadAheadLength = 10240;


class EngineBufferScaleLinear : public EngineBufferScale  {
    Q_OBJECT;
  public:
    EngineBufferScaleLinear(QObject *pParent = nullptr);
    EngineBufferScaleLinear(ReadAheadManager *pReadAheadManager, QObject *pParent);
    virtual ~EngineBufferScaleLinear() override;

    double getScaled(CSAMPLE* pOutput, const int iBufferSize) override;
    void clear() override;

    void setScaleParameters(double base_rate,double* pTempoRatio,double* pPitchRatio) override;

  private:
    int do_scale(CSAMPLE* buf, const int buf_size);
    int do_copy(CSAMPLE* buf, const int buf_size);

    bool m_bClear{false};
    double m_dRate{1.0};
    double m_dOldRate{1.0};

    // Buffer for handling calls to ReadAheadManager
    std::unique_ptr<CSAMPLE[]> m_bufferInt;
    int m_bufferIntSize{0};
    CSAMPLE m_floorSampleOld[2] = { 0.f, 0.f};
    // The read-ahead manager that we use to fetch samples
    double m_dCurrentFrame{0.0};
    double m_dNextFrame{0.0};
};
