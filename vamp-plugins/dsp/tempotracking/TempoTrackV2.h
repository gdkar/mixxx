/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008-2009 Matthew Davies and QMUL.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef TEMPOTRACKV2_H
#define TEMPOTRACKV2_H

#include <vector>
using namespace std;

//!!! Question: how far is this actually sample rate dependent?  I
// think it does produce plausible results for e.g. 48000 as well as
// 44100, but surely the fixed window sizes and comb filtering will
// make it prefer float or half time when run at e.g. 96000?

class TempoTrackV2
{
public:
    /**
     * Construct a tempo tracker that will operate on beat detection
     * function data calculated from audio at the given sample rate
     * with the given frame increment.
     *
     * Currently the sample rate and increment are used only for the
     * conversion from beat frame location to bpm in the tempo array.
     */
    TempoTrackV2(float sampleRate, size_t dfIncrement);
    ~TempoTrackV2();

    // Returned beat periods are given in df increment units; inputtempo and tempi in bpm
    void calculateBeatPeriod(const vector<float> &df,
                             vector<float> &beatPeriod,
                             vector<float> &tempi) {
        calculateBeatPeriod(df, beatPeriod, tempi, 120.0, false);
    }

    // Returned beat periods are given in df increment units; inputtempo and tempi in bpm
    // MEPD 28/11/12 Expose inputtempo and constraintempo parameters
    // Note, if inputtempo = 120 and constraintempo = false, then functionality is as it was before
    void calculateBeatPeriod(const vector<float> &df,
                             vector<float> &beatPeriod,
                             vector<float> &tempi,
                             float inputtempo, bool constraintempo);

    // Returned beat positions are given in df increment units
    void calculateBeats(const vector<float> &df,
                        const vector<float> &beatPeriod,
                        vector<float> &beats) {
        calculateBeats(df, beatPeriod, beats, 0.9, 4.0);
    }

    // Returned beat positions are given in df increment units
    // MEPD 28/11/12 Expose alpha and tightness parameters
    // Note, if alpha = 0.9 and tightness = 4, then functionality is as it was before
    void calculateBeats(const vector<float> &df,
                        const vector<float> &beatPeriod,
                        vector<float> &beats,
                        float alpha, float tightness);

private:
    typedef vector<int> i_vec_t;
    typedef vector<vector<int> > i_mat_t;
    typedef vector<float> d_vec_t;
    typedef vector<vector<float> > d_mat_t;

    float m_rate;
    size_t m_increment;

    void adapt_thresh(d_vec_t &df);
    float mean_array(const d_vec_t &dfin, int start, int end);
    void filter_df(d_vec_t &df);
    void get_rcf(const d_vec_t &dfframe, const d_vec_t &wv, d_vec_t &rcf);
    void viterbi_decode(const d_mat_t &rcfmat, const d_vec_t &wv,
                        d_vec_t &bp, d_vec_t &tempi);
    float get_max_val(const d_vec_t &df);
    int get_max_ind(const d_vec_t &df);
    void normalise_vec(d_vec_t &df);
};

#endif
