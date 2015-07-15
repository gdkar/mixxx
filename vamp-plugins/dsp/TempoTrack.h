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

#ifndef TEMPOTRACK_H
#define TEMPOTRACK_H


#include <cstdio>
#include <vector>
#include <memory>
#include <utility>
#include "DFProcess.h"
#include "Correlation.h"
#include "Framer.h"



using std::vector;
using std::unique_ptr;
using std::make_unique;
struct WinThresh
{
    unsigned int pre;
    unsigned int  post;
};
struct TTParams
{
    unsigned int winLength; //Analysis window length
    unsigned int lagLength; //Lag & Stride size
    unsigned int alpha; //alpha-norm parameter
    unsigned int LPOrd; // low-pass Filter order
    float* LPACoeffs; //low pass Filter den coefficients
    float* LPBCoeffs; //low pass Filter num coefficients
    WinThresh WinT;//window size in frames for adaptive thresholding [pre post]:
};
class TempoTrack  
{
public:
    TempoTrack( TTParams Params );
    virtual ~TempoTrack();
    vector<int> process( vector <float> DF, vector <float> *tempoReturn = 0);
private:
    void initialise( TTParams Params );
    void deInitialise();

    int beatPredict( unsigned int FSP, float alignment, float period, unsigned int step);
    int phaseMM( float* DF, float* weighting, unsigned int winLength, float period );
    void createPhaseExtractor( float* Filter, unsigned int winLength,  float period,  unsigned int fsp, unsigned int lastBeat );
    int findMeter( float* ACF,  unsigned int len, float period );
    void constDetect( float* periodP, int currentIdx, int* flag );
    void stepDetect( float* periodP, float* periodG, int currentIdx, int* flag );
    void createCombFilter( float* Filter, unsigned int winLength, unsigned int TSig, float beatLag );
    float tempoMM( float* ACF, float* weight, int sig );
    unsigned int m_dataLength;
    unsigned int m_winLength;
    unsigned int m_lagLength;
    float		 m_rayparam;
    float		 m_sigma;
    float		 m_DFWVNnorm;
    vector<int>	 m_beats; // Vector of detected beats
    float m_lockedTempo;
    unique_ptr<float[]>m_tempoScratch;
    unique_ptr<float[]> m_smoothRCF; // Smoothed Output of Comb Filterbank (m_tempoScratch)
    // Processing Buffers 
    unique_ptr<float[]> m_rawDFFrame; // Original Detection Function Analysis Frame
    unique_ptr<float[]> m_smoothDFFrame; // Smoothed Detection Function Analysis Frame
    unique_ptr<float[]> m_frameACF; // AutoCorrelation of Smoothed Detection Function 
    //Low Pass Coefficients for DF Smoothing
    float* m_ACoeffs;
    float* m_BCoeffs;
    // Objetcs/operators declaration
    Framer m_DFFramer;
    DFProcess m_DFConditioning;
    Correlation m_correlator;
    // Config structure for DFProcess
    DFProcConfig m_DFPParams;
	// also want to smooth m_tempoScratch 
    DFProcess m_RCFConditioning;
    // Config structure for RCFProcess
    DFProcConfig m_RCFPParams;

};
#endif
