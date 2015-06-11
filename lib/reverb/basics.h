/*
	basics.h

	Copyright 2004-12 Tim Goetze <tim@quitte.de>

	http://quitte.de/dsp/

	Common constants, typedefs, utility functions
	and simplified LADSPA #defines.

	Some code removed by Owen Williams for port to Mixxx, mostly ladspa-specific
	defines and i386 customizations.

*/
/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 3
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA or point your web browser to http://www.gnu.org.
*/

#ifndef _BASICS_H_
#define _BASICS_H_

// NOTE(rryan): 3/2014 Added for MSVC support. (missing M_PI)
#define _USE_MATH_DEFINES
#include <cmath>

#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <float.h>

#include <assert.h>
#include <stdio.h>

#include "util/types.h"
typedef CSAMPLE sample_t;

// NOTE(rryan): 3/2014 Added these for the MSVC build.
#include <QtGlobal>
typedef qint8 int8;
typedef quint8 uint8;
typedef qint16 int16;
typedef quint16 uint16;
typedef qint32 int32;
typedef quint32 uint32;
typedef qint64 int64;
typedef quint64 uint64;

#define MIN_GAIN .000001 /* -120 dB */
/* smallest non-denormal 32 bit IEEE float is 1.18e-38 */
#define NOISE_FLOOR .00000000000005 /* -266 dB */

/* //////////////////////////////////////////////////////////////////////// */
template <class T>
inline T clamp (T value, T lower, T upper)
{
  const T b0 = ( value < lower ) ? lower : value;
  return ( b0 > upper ) ? upper : b0;
}

static inline float
frandom()
{
	return (float) rand() / (float) RAND_MAX;
}

/* lovely algorithm from
  http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
*/
inline uint
next_power_of_2 (uint n)
{
	assert (n <= 0x40000000);

	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;

	return ++n;
}

inline double
db2lin (double db){return pow(10, db*.05);}

inline double
lin2db (double lin){return 20*log10(lin);}

/* //////////////////////////////////////////////////////////////////////// */

#endif /* _BASICS_H_ */
