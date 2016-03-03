/***************************************************************************

                          enginebufferscalest.h  -  description
                             -------------------

    begin                : November 2004
    copyright            : (C) 2004 by Tue Haste Andersen
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

/**
  *@author Tue Haste Andersen
*/

_Pragma("once")
#include "engine/enginebufferscale.h"

// Number of samples to read ahead. Setting this too high (10000) causes
// stuttering.
const int kiSoundTouchReadAheadLength = 1000;
class ReadAheadManager;

namespace soundtouch {
class SoundTouch;
}  // namespace soundtouch

// Uses libsoundtouch to scale audio.
class EngineBufferScaleST : public EngineBufferScale {
    Q_OBJECT
  public:
    EngineBufferScaleST(ReadAheadManager* pReadAheadManager, QObject *pParent);
    EngineBufferScaleST(QObject *pParent = nullptr);
    virtual ~EngineBufferScaleST() override;
    void setScaleParameters(double base_rate,
                            double* pTempoRatio,
                            double* pPitchRatio) override;
    void setSampleRate(int iSampleRate) override;
    // Scale buffer.
    double getScaled(CSAMPLE* pOutput, const int iBufferSize) override;
    // Flush buffer.
    void clear() override;
  private:
    // Holds the playback direction.
    bool m_bBackwards{false};
    // Temporary buffer for reading from the RAMAN.
    std::unique_ptr<CSAMPLE[]> buffer_back;
    // SoundTouch time/pitch scaling lib
    std::unique_ptr<soundtouch::SoundTouch> m_pSoundTouch;
};
