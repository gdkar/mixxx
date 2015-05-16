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

#include "MathUtilities.h"

#include <algorithm>
#include <iostream>
#include <cstring>
#include <cmath>


double MathUtilities::mod(double x, double y){return  x - ( y * floor(x/y) );}
double MathUtilities::princarg(double ang){return mod( ang + M_PI, -2 * M_PI ) + M_PI;}
void MathUtilities::getAlphaNorm(const double *data, unsigned int len, unsigned int alpha, double* ANorm){
    unsigned int i;
    double temp = 0.0;
    double a=0.0;
    for( i = 0; i < len; i++){
	temp = data[ i ];
	a  += ::pow( fabs(temp), double(alpha) );
    }
    a /= ( double )len;
    a = ::pow( a, ( 1.0 / (double) alpha ) );
    *ANorm = a;
}

double MathUtilities::getAlphaNorm( const std::vector <double> &data, unsigned int alpha ){
    unsigned int i;
    unsigned int len = data.size();
    double temp = 0.0;
    double a=0.0;
    for( i = 0; i < len; i++)
    {
	temp = data[ i ];
	a  += ::pow( fabs(temp), double(alpha) );
    }
    a /= ( double )len;
    a = ::pow( a, ( 1.0 / (double) alpha ) );
    return a;
}

double MathUtilities::round(double x){
    return (double)floor(x + 0.5);
}
double MathUtilities::median(const double *src, unsigned int len){
    unsigned int i, j;
    double tmp = 0.0;
    double tempMedian;
    double medianVal;
 
    double* scratch = new double[ len ];//Vector < double > sortedX = Vector < double > ( size );
    std::memmove(&scratch[0],&src[0],len*sizeof(scratch[0]));
    for ( i = 0; i < len - 1; i++ ){
	for ( j = 0; j < len - 1 - i; j++ ){
	    if ( scratch[j + 1] < scratch[j] ){
		// compare the two neighbors
		tmp = scratch[j]; // swap a[j] and a[j+1]
		scratch[j] = scratch[j + 1];
		scratch[j + 1] = tmp;
	    }
	}
    }
    int middle;
    if ( len % 2 == 0 ){
	middle = len / 2;
	tempMedian = ( scratch[middle] + scratch[middle - 1] ) / 2;
    }else{
	middle = ( int )floor( len / 2.0 );
	tempMedian = scratch[middle];
    }
    medianVal = tempMedian;
    delete [] scratch;
    return medianVal;
}
double MathUtilities::sum(const double *src, unsigned int len){
    unsigned int i ;
    double retVal =0.0;
    for(  i = 0; i < len; i++) {retVal += src[ i ];}
    return retVal;
}

double MathUtilities::mean(const double *src, unsigned int len){
    double retVal =0.0;
    double s = sum( src, len );
    retVal =  s  / (double)len;
    return retVal;
}
double MathUtilities::mean(const std::vector<double> &src,
                           unsigned int start,
                           unsigned int count){
    double sum = 0.;
    for (int i = 0; i < count; ++i){sum += src[start + i];}
    return sum / count;
}
void MathUtilities::getFrameMinMax(const double *data, unsigned int len, double *min, double *max)
{
    unsigned int i;
    double temp = 0.0;
    double a=0.0;

    if (len == 0) {
        *min = *max = 0;
        return;
    }
	
    double tmin = data[0], tmax=data[0];
    for( i = 0; i < len; i++){
	temp = data[ i ];
        tmin  = (temp<tmin)?temp:tmin;
        tmax  = (temp>tmax)?temp:tmax;
    }
    *min = tmin;
    *max = tmax;
}
int MathUtilities::getMax( double* pData, unsigned int Length, double* pMax ){
	unsigned int index = 0;
	unsigned int i;
	double temp = 0.0;
	double max = pData[0];
	for( i = 1; i < Length; i++){
		temp = pData[ i ];
                index = (temp>max)?i:index;
                max  = (temp>max)?temp:max;
   	}
	if (pMax) *pMax = max;
	return index;
}
int MathUtilities::getMax( const std::vector<double> & data, double* pMax ){
	unsigned int index = 0;
	unsigned int i;
	double temp = 0.0;
	double max = data[0];
	for( i = 1; i < data.size(); i++){
		temp  = data[ i ];
                index = (temp>max)?i:index;
                max   = (temp>max)?temp:max;
   	}
	if (pMax) *pMax = max;
	return index;
}
void MathUtilities::circShift( double* pData, int length, int shift){
	shift = shift % length;
	double temp;
	int i,n;
	for( i = 0; i < shift; i++){
		temp=*(pData + length - 1);
		for( n = length-2; n >= 0; n--){*(pData+n+1)=*(pData+n);}
        *pData = temp;
    }
}
int MathUtilities::compareInt (const void * a, const void * b){return ( *(int*)a - *(int*)b );}
void MathUtilities::normalise(double *data, int length, NormaliseType type){
    switch (type) {
    case NormaliseNone: return;
    case NormaliseUnitSum:{
        double sum = 0.0;
        for (int i = 0; i < length; ++i) {sum += data[i];}
        double isum = 1.0/sum;
        if (sum != 0.0) {
            for (int i = 0; i < length; ++i) {
                data[i] *= isum;
            }
        }
    }
    break;
    case NormaliseUnitMax:{
        double max = 0.0;
        for (int i = 0; i < length; ++i) {
            if (fabs(data[i]) > max) {
                max = fabs(data[i]);
            }
        }
        double imax = 1.0/max;
        if (max != 0.0) {
            for (int i = 0; i < length; ++i) {
                data[i] *= max;
            }
        }
    }
    break;
    }
}
void MathUtilities::normalise(std::vector<double> &data, NormaliseType type){
    switch (type) {
    case NormaliseNone: return;
    case NormaliseUnitSum:{
        double sum = 0.0;
        for (int i = 0; i < data.size(); ++i) sum += data[i];
        double isum = 1.0/sum;
        if (sum != 0.0) {
            for (int i = 0; i < data.size(); ++i) data[i] *= sum;
        }
    }
    break;
    case NormaliseUnitMax:{
        double max = 0.0;
        for (int i = 0; i < data.size(); ++i) {
            if (fabs(data[i]) > max) max = fabs(data[i]);
            }
        double imax = 1.0/max;
        if (max != 0.0) {
            for (int i = 0; i < data.size(); ++i) data[i] *= imax;
        }
    }
    break;
    }
}
void MathUtilities::adaptiveThreshold(std::vector<double> &data){
    int sz = int(data.size());
    if (sz == 0) return;
    std::vector<double> smoothed(sz);
    int p_pre = 8;
    int p_post = 7;
    for (int i = 0; i < sz; ++i) {
        int first = std::max(0,      i - p_pre);
        int last  = std::min(sz - 1, i + p_post);
        smoothed[i] = mean(data, first, last - first + 1);
    }
    for (int i = 0; i < sz; i++) {
        data[i] -= smoothed[i];
        if (data[i] < 0.0) data[i] = 0.0;
    }
}
bool
MathUtilities::isPowerOfTwo(int x){return (x>1)&&((x&(x-1)));}
int
MathUtilities::nextPowerOfTwo(int x){
    if (isPowerOfTwo(x)) return x;
    int n = 1;
    while (x) { x >>= 1; n <<= 1; }
    return n;
}
int
MathUtilities::previousPowerOfTwo(int x){
    if (isPowerOfTwo(x)) return x;
    int n = 1;
    x >>= 1;
    while (x) { x >>= 1; n <<= 1; }
    return n;
}
int
MathUtilities::nearestPowerOfTwo(int x){
    if (isPowerOfTwo(x)) return x;
    int n0 = previousPowerOfTwo(x), n1 = nearestPowerOfTwo(x);
    if (x - n0 < n1 - x) return n0;
    else return n1;
}
