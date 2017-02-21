/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP library
    Centre for Digital Music, Queen Mary, University of London.
    This file Copyright 2006 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Pitch.h"

#include <cmath>
#include <numeric>

float
Pitch::getFrequencyForPitch(int midiPitch,
			    float centsOffset,
			    float concertA)
{
    auto p = float(midiPitch) + (centsOffset *1e-2f);
    return concertA * std::pow(2.0f, (p - 69.0f) / 12.0f);
}

int
Pitch::getPitchForFrequency(float frequency,
			    float *centsOffsetReturn,
			    float concertA)
{
    auto p = 12.0f * (std::log(frequency / (concertA / 2.0f)) / std::log(2.0f)) + 57.0f;

    auto midiPitch = int(p + 0.00001);
    auto centsOffset = (p - midiPitch) * 100.0f;

    if (centsOffset >= 50.0f) {
	midiPitch = midiPitch + 1;
	centsOffset = -(100.0f - centsOffset);
    }

    if (centsOffsetReturn)
        *centsOffsetReturn = centsOffset;
    return midiPitch;
}

