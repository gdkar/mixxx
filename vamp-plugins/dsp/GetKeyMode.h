/*
 * Author: c.landone 
 * Description:
 *
 * Syntax: C++
 *
 * Copyright (c) 2005 Centre for Digital Music ( C4DM )
 *                    Queen Mary Univesrity of London
 *
 *
 * This program is not free software; you cannot redistribute it 
 * without the explicit authorization from the centre for digital music,
 * queen mary university of london 
 *
 */

#ifndef GETKEYMODE_H
#define GETKEYMODE_H


#include "Decimator.h"
#include "Chromagram.h"


class GetKeyMode  
{
public:
	GetKeyMode( int sampleRate, float tuningFrequency,
		    float hpcpAverage, float medianAverage );

	virtual ~GetKeyMode();

	int process( float* PCMData );

	float krumCorr( float* pData1, float* pData2, size_t length );

	size_t getBlockSize() { return m_ChromaFrameSize*m_DecimationFactor; }
	size_t getHopSize() { return m_ChromaHopSize*m_DecimationFactor; }
	float* getChroma() { return m_ChrPointer; }
	size_t getChromaSize() { return m_BPO; }
	float* getMeanHPCP() { return m_MeanHPCP; }
	float *getKeyStrengths() { return m_keyStrengths; }
	bool isModeMinor( int key ); 

protected:

	float       m_hpcpAverage;
	float       m_medianAverage;
	size_t       m_DecimationFactor;

	//Decimator (fixed)
  std::unique_ptr<Decimator> m_Decimator;
	//chroma configuration
	ChromaConfig m_ChromaConfig;
	//Chromagram object
  std::unique_ptr<Chromagram> m_Chroma;
	//Chromagram output pointer
	float* m_ChrPointer;
	//Framesize
	size_t m_ChromaFrameSize;
	//Hop
	size_t m_ChromaHopSize;
	//Bins per octave
	size_t m_BPO;

	size_t m_ChromaBuffersize;
	size_t m_MedianWinsize;
	
	size_t m_bufferindex;
	size_t m_ChromaBufferFilling;
	size_t m_MedianBufferFilling;
	

	float* m_DecimatedBuffer;
	float* m_ChromaBuffer;
	float* m_MeanHPCP;

	float* m_MajCorr;
	float* m_MinCorr;
	float* m_Keys;
	int* m_MedianFilterBuffer;
	int* m_SortedBuffer;

	float *m_keyStrengths;
};

#endif // !defined GETKEYMODE_H
