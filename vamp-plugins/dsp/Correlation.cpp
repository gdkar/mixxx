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

#include "Correlation.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Correlation::Correlation()
{

}

Correlation::~Correlation()
{

}

void Correlation::doAutoUnBiased(double *src, double *dst, unsigned int length){
    double outVal = 0.0;
    unsigned int i,j;
    for( i = 0; i <  length; i++){
        double acc = 0;
	for( j = i; j < length; j++) {acc += src[ j-i ] * src[ j ]; }
	outVal = acc / ( length - i );
        dst[ i ] = outVal<=0?EPS:outVal;
    }
}
