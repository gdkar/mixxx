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
/*
 * Win32 and the Visual C++ compiler combination do not implement the lrint or lrintf functions. 
 * The float_cast.h header file from http://www.mega-nerd.com/FPcast/
 * contains these functions. 
 * The following define statement is copyrighted by
 * (C) 2001 Erik de Castro Lopo <erikd AT mega-nerd DOT com>
 * The author states
 * "Permission to use, copy, modify, distribute, and sell this file for any 
 *  purpose is hereby granted without fee, provided that the above copyright 
 *  and this permission notice appear in all copies.  No representations are
 *  made about the suitability of this software for any purpose.  It is 
 *  provided "as is" without express or implied warranty."
 */
#if (defined (__WINDOWS__))
    #include	<math.h>
    
    #ifdef WIN64
        #define	lrint(dbl)		((int)(dbl))
        #define	lrintf(flt)		((int)(flt))
    #else

        /*	Win32 doesn't seem to have these functions. 
        **	Therefore implement inline versions of them here.
        */
        
        __inline long int 
        lrint (float flt) 
        {	int intgr;

            _asm
            {	fld flt
                fistp intgr
                } ;
                
            return intgr ;
        } 
        
        __inline long int 
        lrintf (float flt)
        {	int intgr;

            _asm
            {	fld flt
                fistp intgr
                } ;
                
            return intgr ;
        }
    #endif
#endif

class GetKeyMode  
{
public:
	GetKeyMode( int sampleRate, float tuningFrequency,
		    float hpcpAverage, float medianAverage );

	virtual ~GetKeyMode();

	int process( float* PCMData );

	float krumCorr( float* pData1, float* pData2, unsigned int length );

	unsigned int getBlockSize() { return m_ChromaFrameSize*m_DecimationFactor; }
	unsigned int getHopSize() { return m_ChromaHopSize*m_DecimationFactor; }

	float* getChroma() { return m_ChrPointer; }
	unsigned int getChromaSize() { return m_BPO; }

	float* getMeanHPCP() { return m_MeanHPCP; }

	float *getKeyStrengths() { return m_keyStrengths; }

	bool isModeMinor( int key ); 

protected:

	float m_hpcpAverage;
	float m_medianAverage;
	unsigned int m_DecimationFactor;

	//Decimator (fixed)
	Decimator* m_Decimator;

	//chroma configuration
	ChromaConfig m_ChromaConfig;

	//Chromagram object
	Chromagram* m_Chroma;

	//Chromagram output pointer
	float* m_ChrPointer;

	//Framesize
	unsigned int m_ChromaFrameSize;
	//Hop
	unsigned int m_ChromaHopSize;
	//Bins per octave
	unsigned int m_BPO;


	unsigned int m_ChromaBuffersize;
	unsigned int m_MedianWinsize;
	
	unsigned int m_bufferindex;
	unsigned int m_ChromaBufferFilling;
	unsigned int m_MedianBufferFilling;
	

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
