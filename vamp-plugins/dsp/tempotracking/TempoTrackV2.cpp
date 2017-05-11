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

#include "TempoTrackV2.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

#include "maths/MathUtilities.h"
#include <valarray>

static constexpr float EPS = 1e-6f;
//#define   EPS float(1e-6) // just some arbitrary small number

TempoTrackV2::TempoTrackV2(float rate, size_t increment, size_t frame_size) :
    m_rate(rate), m_increment(increment), m_frame_size{frame_size} { }
TempoTrackV2::~TempoTrackV2() { }

void
TempoTrackV2::filter_df(d_vec_t &df)
{
    //equivalent in matlab to [b,a] = butter(2,0.4);
    float a[] = {1.0000f, -0.3695f,0.1958f};
    float b[] = {0.2066f, 0.4131f, 0.2066f};
    for(auto ignored : { 0,1}) {
        void(sizeof(ignored));
        auto inp1 = 0.f, inp2 = 0.f, out1 = 0.f, out2 = 0.f;
        // forwards filtering
        for (auto & dfv : df) {
            auto val =  b[0]* dfv + b[1]*inp1 + b[2]*inp2 - a[1]*out1 - a[2]*out2;
            inp2 = inp1;inp1 = dfv;out2 = out1;out1 = dfv = val;//lp_df[i];
        }
        // copy forwards filtering to df...
        // but, time-reversed, ready for backwards filtering

        std::reverse(std::begin(df),std::end(df));
    }
}
namespace {
constexpr decltype(auto) sqr(auto && t) { return t * t;}
template<class T>
constexpr T gaussian(T x, T sigma, T mu)
{
    return std::exp(-T(0.5) * sqr((x - mu)/sigma)) / std::sqrt(2*T(M_PI)*sqr(sigma));
}
template<class T>
constexpr T rayleigh(T x, T sigma, T mu)
{
    return x * std::exp(-T(0.5) * sqr((x-mu)/sigma)) / sqr(sigma);
}
template<class C>
void normalize(C &c)
{
    using T = typename C::value_type;
    auto acc = std::accumulate(std::begin(c),std::end(c),T{}) + EPS;
    std::transform(std::begin(c),std::end(c),std::begin(c),[inv=(1/acc)](auto && x){return x * inv;});
}
}
// MEPD 28/11/12
// This function now allows for a user to specify an inputtempo (in BPM)
// and a flag "constraintempo" which replaces the general rayleigh weighting for periodicities
// with a gaussian which is centered around the input tempo
// Note, if inputtempo = 120 and constraintempo = false, then functionality is
// as it was before
void
TempoTrackV2::calculateBeatPeriod(const vector<float> &df,
                                  vector<float > &beat_period,
                                  vector<float > &tempi,
                                  float inputtempo, bool constraintempo)
{
    // to follow matlab.. split into 512 sample frames with a 128 hop size
    // calculate the acf,
    // then the rcf.. and then stick the rcfs as columns of a matrix
    // then call viterbi decoding with weight vector and transition matrix
    // and get best path
    //
    // beat tracking frame size (roughly 6 seconds) and hop (1.5 seconds)
    auto idealFrameTime = 6.0f;
    auto idealHopTime   = 1.5f;

    auto incrementTime = m_increment * (1.0f/m_rate);

    auto step   = size_t(idealHopTime / incrementTime);
    step        = std::max<size_t>(128ul,MathUtilities::nearestPowerOfTwo(step));

    auto winlen = size_t(idealFrameTime/incrementTime);
    winlen      = std::max<size_t>(step * 4ul,MathUtilities::nearestPowerOfTwo(winlen));

//    size_t wv_len = 128ul;

    auto wv_len = step;
    // MEPD 28/11/12
    // the default value of inputtempo in the beat tracking plugin is 120
    // so if the user specifies a different inputtempo, the rayparam will be updated
    // accordingly.
    // note: 60*44100/512 is a magic number
    // this might (will?) break if a user specifies a different frame rate for the onset detection function
    auto rayparam = (60.0f * m_rate/inputtempo) / winlen;

    // these debug statements can be removed.
//    std::cerr << "inputtempo" << inputtempo << std::endl;
//    std::cerr << "rayparam" << rayparam << std::endl;
//    std::cerr << "constraintempo" << constraintempo << std::endl;

    // make rayleigh weighting curve
    auto wv = d_vec_t{};
    wv.resize(wv_len);

    // check whether or not to use rayleigh weighting (if constraintempo is false)
    // or use gaussian weighting it (constraintempo is true)
    if (constraintempo) {
        for (auto i=0ul; i<wv.size(); i++) {
            wv[i] = gaussian(float(i), rayparam*0.25f, rayparam);
            // MEPD 28/11/12
            // do a gaussian weighting instead of rayleigh
//            wv[i] = exp( (-0.5*sqr(double(i)-rayparam)/rayparam/4.) );
        }
    } else {
        for (auto i=0ul; i<wv.size(); i++) {
            // MEPD 28/11/12
            // standard rayleigh weighting over periodicities
            wv[i] = rayleigh(float(i),rayparam, 0.0f);
        }
    }
    normalize(wv);
    // matrix to store output of comb filter bank, increment column of matrix at each frame
    d_mat_t rcfmat;
    auto col_counter = -1;

    // main loop for beat period calculation
    for (unsigned int i=0; i+winlen<df.size(); i+=step) {
        // get dfframe
        auto dfframe = d_vec_t(std::begin(df) + i, std::begin(df) + i + winlen);
        // get rcf vector for current frame
        auto rcf = d_vec_t(wv_len);
        get_rcf(dfframe,wv,rcf);
        rcfmat.push_back(rcf);
        col_counter++;
    }

    // now call viterbi decoding function
    viterbi_decode(rcfmat,wv,beat_period,tempi);
}


void
TempoTrackV2::get_rcf(const d_vec_t &dfframe_in, const d_vec_t &wv, d_vec_t &rcf)
{
    // calculate autocorrelation function
    // then rcf
    // just hard code for now... don't really need separate functions to do this

    // make acf

    auto dfframe = dfframe_in;
    MathUtilities::adaptiveThreshold(dfframe);

    auto acf = d_vec_t(dfframe.size());

    for (auto it = dfframe.begin(),at = acf.begin(); it != dfframe.end(); ++it) {
        auto sum = std::inner_product(it,dfframe.end(),dfframe.begin(),0.0f);
        *at++ = float(sum)/(dfframe.end() - it);
    }
    // now apply comb filtering
    auto numelem = 4;
    for (auto i = 1ul;i + 1ul < rcf.size();++i){ // max beat period
        auto at = std::begin(acf) + i;
        for (int a = 0,l = 1;a < numelem;(++a),(l+=2),(at+=i)) // number of comb elements
            rcf[i] += std::accumulate(at, at + l, 0.f) / l;
        rcf[i] *= wv[i];
    }
    // apply adaptive threshold to rcf
    MathUtilities::adaptiveThreshold(rcf);
    {
        auto rcfsum =0.f;
        for(auto & rcfv : rcf)
            rcfsum += (rcfv += EPS);
        auto rcfi = 1/(rcfsum + EPS);
        for(auto &rcfv : rcf)
            rcfv *= rcfi;
    }
}

void
TempoTrackV2::viterbi_decode(
    const d_mat_t &rcfmat
  , const d_vec_t &wv
  , d_vec_t &beat_period
  , d_vec_t &tempi)
{
    // following Kevin Murphy's Viterbi decoding to get best path of
    // beat periods through rfcmat
    auto T = rcfmat.size();
    if (T < 2)
        return; // can't do anything at all meaningful

    auto Q = rcfmat.front().size();
    // make transition matrix
    auto tmat = d_mat_t(wv.size(),d_vec_t(wv.size(),0.f));
    // variance of Gaussians in transition matrix
    // formed of Gaussians on diagonal - implies slow tempo change
    auto sigma = 2.0f;
    // don't want really short beat periods, or really long ones
    for (auto i=20ul;i <wv.size()-20ul; ++i) {
        for (auto j=20ul; j<wv.size()-20ul; ++j)
            tmat[i][j] = gaussian(float(j),sigma,float(i));
        normalize(tmat[i]);
    }

    // parameters for Viterbi decoding... this part is taken from
    // Murphy's matlab
    auto delta = d_mat_t(T, d_vec_t(Q,0.f));
    auto psi   = i_mat_t(T, i_vec_t(Q,0));

    // initialize first column of delta
    std::transform(
        std::begin(wv)
       ,std::end(wv)
       ,std::begin(rcfmat.front())
       ,std::begin(delta.front())
       ,std::multiplies<void>{});

    normalize(delta[0]);

    for (auto t=1ul; t<T; t++) {
        for (auto j=0ul; j<Q; j++) {
            auto maxi = j;
            auto maxv = delta[t-1][j] * tmat[j][j];
            for(auto i = 0ul; i < Q; ++i) {
                auto thisv = delta[t-1][i] * tmat[j][i];
                if(thisv > maxv)
                    maxv = thisv,maxi = i;
            }
            delta[t][j] = maxv * rcfmat[t][j];
            psi  [t][j] = maxi;
        }
        normalize(delta[t]);
    }
    auto bestpath = i_vec_t(T);

    // find starting point - best beat period for "last" frame
    bestpath.back() = std::max_element(
        std::begin(delta.back())
      , std::end(delta.back())
        ) - std::begin(delta.back());

    // backtrace through index of maximum values in psi
    for (auto t=T-1; t-- ;)
        bestpath[t] = psi[t+1][bestpath[t+1]];

    auto step = wv.size();
    auto per = std::begin(beat_period);
    for (auto pth : bestpath)
        per = std::fill_n(per, step, pth);

    //fill in the last values...
    std::fill(per, std::end(beat_period), *(per-1));

    std::transform(
        std::begin(beat_period)
      , std::end(beat_period)
      , std::back_inserter(tempi)
      , [factor=((60.0f*m_rate)/m_increment)](auto && x){return factor/x;});
}
// MEPD 28/11/12
// this function has been updated to allow the "alpha" and "tightness" parameters
// of the dynamic program to be set by the user
// the default value of alpha = 0.9 and tightness = 4
void
TempoTrackV2::calculateBeats(const vector<float > &df,
                             const vector<float > &beat_period,
                             vector<float > &beats, float alpha, float tightness)
{
    if (df.empty() || beat_period.empty())
        return;

    auto cumscore = d_vec_t(df.size());
    auto backlink = i_vec_t(df.size(), -1);

    // main loop
    for (auto i=0ul; i<df.size(); i++) {
        auto fperiod = beat_period[i];
        auto iperiod = int(fperiod);
        auto xx = int(i) - iperiod;
        if(xx >= 0) {
            auto prange_min = std::max(0,xx - iperiod);
            auto prange_max = xx + iperiod/2;
            // transition range
            auto gen_weight = [tightness,mu=std::log(fperiod),i](auto x)
            {
                return std::exp(-0.5f * sqr(tightness * (std::log(float(i-x)) - mu)));
            };
            auto vv = gen_weight(xx) * cumscore[xx];
            for (auto jj = prange_min;jj<prange_max;++jj) {
                auto cscore = gen_weight(jj) * cumscore[jj];
                if(cscore > vv) {
                    vv = cscore, xx = jj;
                }
            }
            cumscore[i] = alpha * vv + (1-alpha)*df[i];
            backlink[i] = xx;
        }else{
            cumscore[i] = (1-alpha)*df[i];
            backlink[i] = xx;
        }
    }
    // STARTING POINT, I.E. LAST BEAT.. PICK A STRONG POINT IN cumscore VECTOR
    auto it = std::end(cumscore) - std::min<int>(beat_period.back(), cumscore.size());
    auto startpoint = std::max_element(it ,std::end(cumscore)) - std::begin(cumscore);

    // USE BACKLINK TO GET EACH NEW BEAT (TOWARDS THE BEGINNING OF THE FILE)
    //  BACKTRACKING FROM THE END TO THE BEGINNING.. MAKING SURE NOT TO GO BEFORE SAMPLE 0
    auto ibeats = i_vec_t{};
    ibeats.push_back(startpoint);
//    std::cerr << "startpoint = " << startpoint << std::endl;
    while (backlink[ibeats.back()] > 0) {
//        std::cerr << "backlink[" << ibeats.back() << "] = " << backlink[ibeats.back()] << std::endl;
        ibeats.push_back(backlink[ibeats.back()]);
    }
    std::copy(ibeats.rbegin(),ibeats.rend(),std::back_inserter(beats));
}
