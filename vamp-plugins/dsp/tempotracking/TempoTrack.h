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


#include <stdio.h>
#include <vector>

#include "dsp/signalconditioning/DFProcess.h"
#include "maths/Correlation.h"
#include "dsp/signalconditioning/Framer.h"



using std::vector;

struct WinThresh
{
    unsigned int pre;
    unsigned int post;
};

struct TTParams
{
    unsigned int winLength; //Analysis window length
    unsigned int lagLength; //Lag & Stride size
    unsigned int alpha; //alpha-norm parameter
    unsigned int LPOrd; // low-pass Filter order
    double* LPACoeffs; //low pass Filter den coefficients
    double* LPBCoeffs; //low pass Filter num coefficients
    WinThresh WinT;//window size in frames for adaptive thresholding [pre post]:
};


class TempoTrack  
{
public:
    TempoTrack( TTParams Params );
    virtual ~TempoTrack();

    vector<int> process( vector <double> DF, vector <double> *tempoReturn = 0);

	
private:
    void initialise( TTParams Params );
    void deInitialise();

    int beatPredict( unsigned int FSP, double alignment, double period, unsigned int step);
    int phaseMM( double* DF, double* weighting, unsigned int winLength, double period );
    void createPhaseExtractor( double* Filter, unsigned int winLength,  double period,  unsigned int fsp, unsigned int lastBeat );
    int findMeter( double* ACF,  unsigned int len, double period );
    void constDetect( double* periodP, int currentIdx, int* flag );
    void stepDetect( double* periodP, double* periodG, int currentIdx, int* flag );
    void createCombFilter( double* Filter, unsigned int winLength, unsigned int TSig, double beatLag );
    double tempoMM( double* ACF, double* weight, int sig );
	
    unsigned int m_dataLength;
    unsigned int m_winLength;
    unsigned int m_lagLength;

    double		 m_rayparam;
    double		 m_sigma;
    double		 m_DFWVNnorm;

    vector<int>	 m_beats; // Vector of detected beats

    double m_lockedTempo;

    double* m_tempoScratch;
    double* m_smoothRCF; // Smoothed Output of Comb Filterbank (m_tempoScratch)
	
    // Processing Buffers 
    double* m_rawDFFrame; // Original Detection Function Analysis Frame
    double* m_smoothDFFrame; // Smoothed Detection Function Analysis Frame
    double* m_frameACF; // AutoCorrelation of Smoothed Detection Function 

    //Low Pass Coefficients for DF Smoothing
    double* m_ACoeffs;
    double* m_BCoeffs;
	
    // Objetcs/operators declaration
    Framer m_DFFramer;
    DFProcess* m_DFConditioning;
    // Config structure for DFProcess
    DFProcConfig m_DFPParams;

	// also want to smooth m_tempoScratch 
    DFProcess* m_RCFConditioning;
    // Config structure for RCFProcess
    DFProcConfig m_RCFPParams;



};

#endif
