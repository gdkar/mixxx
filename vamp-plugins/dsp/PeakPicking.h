/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file 2005-2006 Christian Landone.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

// PeakPicking.h: interface for the PeakPicking class.
//
//////////////////////////////////////////////////////////////////////

_Pragma("once")
#include "MathUtilities.h"
#include "MathAliases.h"
#include "DFProcess.h"
#include <memory>
struct PPWinThresh
{
    unsigned int pre;
    unsigned int  post;
};
struct QFitThresh
{
    float a;
    float b;
    float c;
};
struct PPickParams
{
    unsigned int length; //Detection FunctionLength
    float tau; // time resolution of the detection function:
    unsigned int alpha; //alpha-norm parameter
    float cutoff;//low-pass Filter cutoff freq
    unsigned int LPOrd; // low-pass Filter order
    float* LPACoeffs; //low pass Filter den coefficients
    float* LPBCoeffs; //low pass Filter num coefficients
    PPWinThresh WinT;//window size in frames for adaptive thresholding [pre post]:
    QFitThresh QuadThresh;
};
class PeakPicking  
{
public:
    PeakPicking( PPickParams Config );
    virtual ~PeakPicking();
    void process( float* src, unsigned int len, vector<int> &onsets  );
private:
    int  quadEval( vector<float> &src, vector<int> &idx );
    DFProcConfig m_DFProcessingParams;
    unsigned int m_DFLength ;
    float Qfilta ;
    float Qfiltb;
    float Qfiltc;
    std::unique_ptr<float[]> m_workBuffer;
    std::unique_ptr<DFProcess>	m_DFSmoothing;
};
