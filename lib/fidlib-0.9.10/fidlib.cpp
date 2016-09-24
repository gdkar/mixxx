#include <cstring>
#include <exception>
#include <system_error>
#include <cerrno>
#include <stdexcept>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <cmath>
#include <complex>

#include "fidlib.hpp"
namespace{
    const char * VERSION = "0.0.0-FUCKIT";
    inline double prewarp(double val) {
        return std::tan(val * M_PI) / M_PI;
    }
    struct Entry {
        FidFilter *(*rout)(Fid *,double,double,double,int,int,double*); // Designer routine address
        const char *fmt;	// Format for spec-string
        const char *txt;	// Human-readable description of filter
    };
    FidFilter*
    do_highpass(Fid* fid, int mz, double freq) {
        fid->highpass(prewarp(freq));
        if (mz)
            fid->s2z_matchedZ();
        else
            fid->s2z_bilinear();
        auto rv= fid->z2fidfilter(1.0, ~0);	// FIR is constant
        rv->val[0]= 1.0 / fid->response(rv, 0.5);
        return rv;
    }
    FidFilter* do_lowpass(Fid* fid, int mz, double freq) {
        fid->lowpass(prewarp(freq));
        if (mz) fid->s2z_matchedZ();
        else fid->s2z_bilinear();
        auto rv= fid->z2fidfilter(1.0, ~0);	// FIR is constant
        rv->val[0]= 1.0 / fid->response(rv, 0);
        return rv;
    }
    FidFilter* do_bandpass(Fid *fid, int mz, double f0, double f1) {
        fid->bandpass(prewarp(f0), prewarp(f1));
        if (mz)
            fid->s2z_matchedZ();
        else
            fid->s2z_bilinear();
        auto rv= fid->z2fidfilter(1.0, ~0);	// FIR is constant
        rv->val[0]= 1.0 / fid->response(rv, fid->search_peak(rv, f0, f1));
        return rv;
    }
    FidFilter* do_bandstop(Fid *fid, int mz, double f0, double f1) {
        fid->bandstop(prewarp(f0), prewarp(f1));
        if (mz)
            fid->s2z_matchedZ();
        else
            fid->s2z_bilinear();
        auto rv= fid->z2fidfilter(1.0, 5);	// FIR second coefficient is *non-const* for bandstop
        rv->val[0]= 1.0 / fid->response(rv, 0.0);	// Use 0Hz response as reference
        return rv;
    }
    FidFilter* des_lpbe(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->bessel(order);
        return do_lowpass(self,BL, f0);
    }
    FidFilter* des_hpbe(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->bessel(order);
        return do_highpass(self,BL, f0);
    }
    FidFilter* des_bpbe(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->bessel(order);
        return do_bandpass(self,BL, f0, f1);
    }
    FidFilter* des_bsbe(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->bessel(order);
        return do_bandstop(self,BL, f0, f1);
    }
    FidFilter* des_lpbez(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->bessel(order);
        return do_lowpass(self,MZ, f0);
    }
    FidFilter* des_hpbez(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->bessel(order);
        return do_highpass(self,MZ, f0);
    }
    FidFilter* des_bpbez(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->bessel(order);
        return do_bandpass(self,MZ, f0, f1);
    }
    FidFilter* des_bsbez(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->bessel(order);
        return do_bandstop(self,MZ, f0, f1);
    }
    FidFilter* des_lpbu(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->butterworth(order);
        return do_lowpass(self,BL, f0);
    }
    FidFilter* des_hpbu(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->butterworth(order);
        return do_highpass(self,BL, f0);
    }
    FidFilter* des_bpbu(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->butterworth(order);
        return do_bandpass(self,BL, f0, f1);
    }
    FidFilter* des_bsbu(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->butterworth(order);
        return do_bandstop(self,BL, f0, f1);
    }
    FidFilter* des_lpbuz(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->butterworth(order);
        return do_lowpass(self,MZ, f0);
    }
    FidFilter* des_hpbuz(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->butterworth(order);
        return do_highpass(self,MZ, f0);
    }
    FidFilter* des_bpbuz(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->butterworth(order);
        return do_bandpass(self,MZ, f0, f1);
    }
    FidFilter* des_bsbuz(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->chebyshev(order, arg[0]);
        return do_lowpass(self,BL, f0);
    }
    FidFilter* des_lpch(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->chebyshev(order, arg[0]);
        return do_lowpass(self,BL, f0);
    }
    FidFilter* des_hpch(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->chebyshev(order, arg[0]);
        return do_highpass(self,BL, f0);
    }
    FidFilter* des_bpch(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->chebyshev(order, arg[0]);
        return do_bandpass(self,BL, f0, f1);
    }
    FidFilter* des_bsch(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->chebyshev(order, arg[0]);
        return do_bandstop(self,BL, f0, f1);
    }
    FidFilter* des_lpchz(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->chebyshev(order, arg[0]);
        return do_lowpass(self,MZ, f0);
    }
    FidFilter* des_hpchz(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->chebyshev(order, arg[0]);
        return do_highpass(self,MZ, f0);
    }
    FidFilter* des_bpchz(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->chebyshev(order, arg[0]);
        return do_bandpass(self,MZ, f0, f1);
    }
    FidFilter* des_bschz(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        self->chebyshev(order, arg[0]);
        return do_bandstop(self,MZ, f0, f1);
    }
    FidFilter* des_lpbq(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto omega= 2 * M_PI * f0;
        auto cosv= std::cos(omega);
        auto alpha= std::sin(omega) / 2 / arg[0];
        return self->stack_filter(order, 3, 7,
                    'I', 0x0, 3, 1 + alpha, -2 * cosv, 1 - alpha,
                    'F', 0x7, 3, 1.0, 2.0, 1.0,
                    'F', 0x0, 1, (1-cosv) * 0.5);
    }
    FidFilter* des_hpbq(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto omega= 2 * M_PI * f0;
        auto cosv= std::cos(omega);
        auto alpha= std::sin(omega) / 2 / arg[0];
        return self->stack_filter(order, 3, 7,
                    'I', 0x0, 3, 1 + alpha, -2 * cosv, 1 - alpha,
                    'F', 0x7, 3, 1.0, -2.0, 1.0,
                    'F', 0x0, 1, (1+cosv) * 0.5);
    }
    FidFilter* des_bpbq(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto omega= 2 * M_PI * f0;
        auto cosv= std::cos(omega);
        auto alpha= std::sin(omega) / 2 / arg[0];
        return self->stack_filter(order, 3, 7,
                    'I', 0x0, 3, 1 + alpha, -2 * cosv, 1 - alpha,
                    'F', 0x7, 3, 1.0, 0.0, -1.0,
                    'F', 0x0, 1, alpha);
    }
    FidFilter* des_bsbq(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto omega= 2 * M_PI * f0;
        auto cosv= std::cos(omega);
        auto alpha= std::sin(omega) / 2 / arg[0];
        return self->stack_filter(order, 2, 6,
                    'I', 0x0, 3, 1 + alpha, -2 * cosv, 1 - alpha,
                    'F', 0x5, 3, 1.0, -2 * cosv, 1.0);
    }
    FidFilter* des_apbq(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto omega= 2 * M_PI * f0;
        auto cosv = std::cos(omega);
        auto alpha= std::sin(omega) / 2 / arg[0];
        return self->stack_filter(order, 2, 6,
                    'I', 0x0, 3, 1 + alpha, -2 * cosv, 1 - alpha,
                    'F', 0x0, 3, 1 - alpha, -2 * cosv, 1 + alpha);
    }
    FidFilter* des_pkbq(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto omega= 2 * M_PI * f0;
        auto cosv = std::cos(omega);
        auto alpha= std::sin(omega) / 2 / arg[0];
        auto A    = std::pow(10, arg[1]/40);
        return self->stack_filter(order, 2, 6,
                    'I', 0x0, 3, 1 + alpha/A, -2 * cosv, 1 - alpha/A,
                    'F', 0x0, 3, 1 + alpha*A, -2 * cosv, 1 - alpha*A);
    }
    FidFilter* des_lsbq(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto omega= 2 * M_PI * f0;
        auto cosv= std::cos(omega);
        auto sinv= std::sin(omega);
        auto A= std::pow(10, arg[1]/40);
        auto beta= std::sqrt((A*A+1)/arg[0] - (A-1)*(A-1));
        return self->stack_filter(order, 2, 6,
                    'I', 0x0, 3,(A+1) + (A-1)*cosv + beta*sinv,-2 * ((A-1) + (A+1)*cosv),(A+1) + (A-1)*cosv - beta*sinv,
                    'F', 0x0, 3,A*((A+1) - (A-1)*cosv + beta*sinv),2*A*((A-1) - (A+1)*cosv),A*((A+1) - (A-1)*cosv - beta*sinv));
    }
    FidFilter* des_hsbq(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto omega= 2 * M_PI * f0;
        auto cosv = std::cos(omega);
        auto sinv = std::sin(omega);
        auto A    = std::pow(10, arg[1]/40);
        auto beta = std::sqrt((A*A+1)/arg[0] - (A-1)*(A-1));
        return self->stack_filter(order, 2, 6,
                    'I', 0x0, 3,   (A+1) - (A-1)*cosv + beta*sinv,    2*((A-1) - (A+1)*cosv),   (A+1) - (A-1)*cosv - beta*sinv,
                    'F', 0x0, 3,A*((A+1) + (A-1)*cosv + beta*sinv),-A*2*((A-1) + (A+1)*cosv),A*((A+1) + (A-1)*cosv - beta*sinv));
    }
    FidFilter* des_lpbl(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto wid= 0.4109205/f0;
        auto tot = 1., adj = 0.;
        auto max= (int)floor(wid);
        auto a = 0;
        auto ff=std::make_unique<FidFilter>();
        ff->typ= 'F';
        ff->resize( max*2+1);
        ff->val[max]= tot= 1.0;
        for (a= 1; a<=max; a++) {
            auto val= 0.42 + 0.5 * std::cos(M_PI * a / wid) +0.08 * std::cos(M_PI * 2.0 * a / wid);
            ff->val[max-a]= val;
            ff->val[max+a]= val;
            tot += val * 2.0;
        }
        adj= 1/tot;
        for (a= 0; a<=max*2; a++)
            ff->val[a] *= adj;
        return ff.release();
    }
    FidFilter* des_lphm(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void)rate;(void)f1;(void)order;(void)n_arg;(void)arg;
        auto wid= 0.3262096/f0;
        auto tot = 0., adj = 0.;
        auto max= (int)floor(wid);
        auto a = 0;
        auto ff=std::make_unique<FidFilter>();
        ff->typ= 'F';
        ff->resize( max*2+1);
        ff->val[max]= tot= 1.0;
        for (a= 1; a<=max; a++) {
            auto val= 0.54 +  0.46 * std::cos(M_PI * a / wid);
            ff->val[max-a]= val;
            ff->val[max+a]= val;
            tot += val * 2.0;
        }
        adj= 1/tot;
        for (a= 0; a<=max*2; a++)
            ff->val[a] *= adj;
        return ff.release();
    }
    FidFilter* des_lphn(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void) rate;(void) f1;(void)order;(void)n_arg;(void)arg;
        auto wid= 0.360144/f0;
        auto tot = 0., adj = 0.;
        auto max= (int)floor(wid);
        auto a = 0;
        auto ff= std::make_unique<FidFilter>();
        ff->typ= 'F';
        ff->resize(max*2+1);
        ff->val[max]= tot= 1.0;
        for (a= 1; a<=max; a++) {
            auto val= 0.5 + 0.5 * std::cos(M_PI * a / wid);
            ff->val[max-a]= val;
            ff->val[max+a]= val;
            tot += val * 2.0;
        }
        adj= 1/tot;
        for (a= 0; a<=max*2; a++)
            ff->val[a] *= adj;
        return ff.release();
    }
    FidFilter* des_lpba(Fid*self,double rate, double f0, double f1, int order, int n_arg, double *arg) {
        (void) self;(void) rate;(void) f1;(void)order;(void)n_arg;(void)arg;
        auto wid= 0.3189435/f0;
        auto tot = 0., adj = 0.;
        auto max= (int)floor(wid);
        auto a = 0;
        auto ff = std::make_unique<FidFilter>();
        ff->typ= 'F';
        ff->resize(max*2+1);
        ff->val[max]= tot= 1.0;
        for (a= 1; a<=max; a++) {
            auto val= 1.0 - a/wid;
            ff->val[max-a]= val;
            ff->val[max+a]= val;
            tot += val * 2.0;
        }
        adj= 1/tot;
        for (a= 0; a<=max*2; a++)
            ff->val[a] *= adj;
        return ff.release();
    }
    Entry filter[] = {
    { &des_lpbe, "LpBe#O/#F", 
        "Lowpass Bessel filter, order #O, -3.01dB frequency #F" },
    { &des_hpbe, "HpBe#O/#F", 
        "Highpass Bessel filter, order #O, -3.01dB frequency #F" },
    { &des_bpbe, "BpBe#O/#R", 
        "Bandpass Bessel filter, order #O, -3.01dB frequencies #R" },
    { &des_bsbe, "BsBe#O/#R", 
        "Bandstop Bessel filter, order #O, -3.01dB frequencies #R" },
    { &des_lpbu, "LpBu#O/#F", 
        "Lowpass Butterworth filter, order #O, -3.01dB frequency #F" },
    { &des_hpbu, "HpBu#O/#F", 
        "Highpass Butterworth filter, order #O, -3.01dB frequency #F" },
    { &des_bpbu, "BpBu#O/#R", 
        "Bandpass Butterworth filter, order #O, -3.01dB frequencies #R" },
    { &des_bsbu, "BsBu#O/#R", 
        "Bandstop Butterworth filter, order #O, -3.01dB frequencies #R" },
    { &des_lpch, "LpCh#O/#V/#F",
        "Lowpass Chebyshev filter, order #O, passband ripple #VdB, -3.01dB frequency #F" },
    { &des_hpch, "HpCh#O/#V/#F", 
        "Highpass Chebyshev filter, order #O, passband ripple #VdB, -3.01dB frequency #F" },
    { &des_bpch, "BpCh#O/#V/#R", 
        "Bandpass Chebyshev filter, order #O, passband ripple #VdB, -3.01dB frequencies #R" },
    { &des_bsch, "BsCh#O/#V/#R", 
        "Bandstop Chebyshev filter, order #O, passband ripple #VdB, -3.01dB frequencies #R" },
    { &des_lpbez, "LpBeZ#O/#F",
        "Lowpass Bessel filter, matched z-transform, order #O, -3.01dB frequency #F" },
    { &des_hpbez, "HpBeZ#O/#F",
        "Highpass Bessel filter, matched z-transform, order #O, -3.01dB frequency #F" },
    { &des_bpbez, "BpBeZ#O/#R",
        "Bandpass Bessel filter, matched z-transform, order #O, -3.01dB frequencies #R" },
    { &des_bsbez, "BsBeZ#O/#R",
        "Bandstop Bessel filter, matched z-transform, order #O, -3.01dB frequencies #R" },
    { &des_lpbuz, "LpBuZ#O/#F",
        "Lowpass Butterworth filter, matched z-transform, order #O, -3.01dB frequency #F" },
    { &des_hpbuz, "HpBuZ#O/#F",
        "Highpass Butterworth filter, matched z-transform, order #O, -3.01dB frequency #F" },
    { &des_bpbuz, "BpBuZ#O/#R",
        "Bandpass Butterworth filter, matched z-transform, order #O, -3.01dB frequencies #R" },
    { &des_bsbuz, "BsBuZ#O/#R",
        "Bandstop Butterworth filter, matched z-transform, order #O, -3.01dB frequencies #R" },
    { &des_lpchz, "LpChZ#O/#V/#F",
        "Lowpass Chebyshev filter, matched z-transform, order #O, "
        "passband ripple #VdB, -3.01dB frequency #F" },
    { &des_hpchz, "HpChZ#O/#V/#F",
        "Highpass Chebyshev filter, matched z-transform, order #O, "
        "passband ripple #VdB, -3.01dB frequency #F" },
    { &des_bpchz, "BpChZ#O/#V/#R",
        "Bandpass Chebyshev filter, matched z-transform, order #O, "
        "passband ripple #VdB, -3.01dB frequencies #R" },
    { &des_bschz, "BsChZ#O/#V/#R",
        "Bandstop Chebyshev filter, matched z-transform, order #O, "
        "passband ripple #VdB, -3.01dB frequencies #R" },
    { &des_lpbq, "LpBq#o/#V/#F",
        "Lowpass biquad filter, order #O, Q=#V, -3.01dB frequency #F" },
    { &des_hpbq, "HpBq#o/#V/#F",
        "Highpass biquad filter, order #O, Q=#V, -3.01dB frequency #F" },
    { &des_bpbq, "BpBq#o/#V/#F",
        "Bandpass biquad filter, order #O, Q=#V, centre frequency #F" },
    { &des_bsbq, "BsBq#o/#V/#F",
        "Bandstop biquad filter, order #O, Q=#V, centre frequency #F" },
    { &des_apbq, "ApBq#o/#V/#F",
        "Allpass biquad filter, order #O, Q=#V, centre frequency #F" },
    { &des_pkbq, "PkBq#o/#V/#V/#F",
        "Peaking biquad filter, order #O, Q=#V, dBgain=#V, frequency #F" },
    { &des_lsbq, "LsBq#o/#V/#V/#F",
        "Lowpass shelving biquad filter, S=#V, dBgain=#V, frequency #F" },
    { &des_hsbq, "HsBq#o/#V/#V/#F",
        "Highpass shelving biquad filter, S=#V, dBgain=#V, frequency #F" },
    { &des_lpbl, "LpBl/#F",
        "Lowpass Blackman window, -3.01dB frequency #F" },
    { &des_lphm, "LpHm/#F",
        "Lowpass Hamming window, -3.01dB frequency #F" },
    { &des_lphn, "LpHn/#F",
        "Lowpass Hann window, -3.01dB frequency #F" },
    { &des_lpba, "LpBa/#F",
        "Lowpass Bartlet (triangular) window, -3.01dB frequency #F" },
    { 0, 0, 0 }
    };
    const double bessel_1[]= {
        -1.00000000000e+00
    };
    const double bessel_2[]= {
        -1.10160133059e+00, 6.36009824757e-01,
    };
    const double bessel_3[]= {
        -1.04740916101e+00, 9.99264436281e-01,
        -1.32267579991e+00,
    };
    const double bessel_4[]= {
        -9.95208764350e-01, 1.25710573945e+00,
        -1.37006783055e+00, 4.10249717494e-01,
    };
    const double bessel_5[]= {
        -9.57676548563e-01, 1.47112432073e+00,
        -1.38087732586e+00, 7.17909587627e-01,
        -1.50231627145e+00,
    };
    const double bessel_6[]= {
        -9.30656522947e-01, 1.66186326894e+00,
        -1.38185809760e+00, 9.71471890712e-01,
        -1.57149040362e+00, 3.20896374221e-01,
    };
    const double bessel_7[]= {
        -9.09867780623e-01, 1.83645135304e+00,
        -1.37890321680e+00, 1.19156677780e+00,
        -1.61203876622e+00, 5.89244506931e-01,
        -1.68436817927e+00,
    };
    const double bessel_8[]= {
        -8.92869718847e-01, 1.99832584364e+00,
        -1.37384121764e+00, 1.38835657588e+00,
        -1.63693941813e+00, 8.22795625139e-01,
        -1.75740840040e+00, 2.72867575103e-01,
    };
    const double bessel_9[]= {
        -8.78399276161e-01, 2.14980052431e+00,
        -1.36758830979e+00, 1.56773371224e+00,
        -1.65239648458e+00, 1.03138956698e+00,
        -1.80717053496e+00, 5.12383730575e-01,
        -1.85660050123e+00,
    };
    const double bessel_10[]= {
        -8.65756901707e-01, 2.29260483098e+00,
        -1.36069227838e+00, 1.73350574267e+00,
        -1.66181024140e+00, 1.22110021857e+00,
        -1.84219624443e+00, 7.27257597722e-01,
        -1.92761969145e+00, 2.41623471082e-01,
    };
    const double * const bessel_poles[] = {
        bessel_1, bessel_2, bessel_3, bessel_4, bessel_5,
        bessel_6, bessel_7, bessel_8, bessel_9, bessel_10,
    };
    //
    //      Complex multiply: aa *= bb;
    //
    void  error(const char *fmt, ...) {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);	// Ignore overflow
        va_end(ap);
        buf[sizeof(buf)-1]= 0;
        // If error handler routine returns, we dump to STDERR and exit anyway
        throw std::runtime_error("FidLib Error: " + std::string(buf));
    }
    std::complex<double> evaluate( double *coef, int n_coef, std::complex<double> in) {
        auto rv = std::complex<double>(*coef++);
        if (--n_coef > 0) {
            // Handle second iteration by hand
            auto pz = in;
            rv += *coef * pz;
            coef++; n_coef--;
            // Loop for remainder
            while (n_coef > 0) {
                pz *= in;
                rv += *coef * pz;
                coef++;
                n_coef--;
            }
        }
        return rv;
    }
    void skipWS(char **pp) {
        auto p= *pp;
        while (*p) {
            if (isspace(*p)) {
                p++;
                continue;
            }
            if (*p == '#') {
                while (*p && *p != '\n') p++;
                continue;
            }
            break;
        }
        *pp= p;
    }
    int grabWord(char **pp, char *buf, size_t buflen) {
        
        skipWS(pp);
        auto p = *pp;
        if (!*p)
            return 0;

        auto q = p;
        if (*q == ',' || *q == ';' || *q == ')' || *q == ']' || *q == '}') {
            q++;
        } else {
            while (*q && *q != '#' && !isspace(*q) &&  (*q != ',' && *q != ';' && *q != ')' && *q != ']' && *q != '}')) {
                q++;
            }
        }
        auto len = q-p;
        if (len >= buflen)
            return 0;
        std::memcpy(buf, p, len);
        buf[len] = '\0';
        *pp = q;
        return 1;
    }
}
//
//      Evaluate a complex polynomial given the coefficients.
//      rv[0]+i*rv[1] is the result, in[0]+i*in[1] is the input value.
//      Coefficients are real values.
//
double 
Fid::response(FidFilter *filt, double freq) {
   auto top = std::complex<double>{1,0};
   auto bot = std::complex<double>{1,0};
   auto zz  = std::polar<double>( 1., freq * 2 * M_PI);

   while (filt && filt->size()) {
      auto resp = evaluate(&filt->val[0], filt->size(), zz);
      if (filt->typ == 'I')
        bot *= resp;
      else if (filt->typ == 'F')
        top *= resp;
      else 
        error("Unknown filter type %d in Fid::response()", filt->typ);
      filt= filt->next();
   }
   return std::abs(top/bot);
}
int 
Fid::calc_delay(FidFilter *filt) {
   double tot, tot100, tot50;
   auto cnt = 0;
   auto run = std::make_unique<Run<double> >(filt);

   // Run through to find at least the 99.9% point of filter; the r2
   // (tot100) filter runs at 4x the speed of the other one to act as
   // a reference point much further ahead in the impulse response.
   auto f1 = std::make_unique<RunBuf<double> >(run.get());
   auto f2 = std::make_unique<RunBuf<double> >(run.get());
   
   tot= std::abs(f1->step(1.0));
   tot100= std::abs(f2->step( 1.0));
   tot100 += std::abs(f2->step( 0.0));
   tot100 += std::abs(f2->step( 0.0));
   tot100 += std::abs(f2->step( 0.0));
   
   for (cnt= 1; cnt < 0x1000000; cnt++) {
      tot += std::abs(f1->step( 0.0));
      tot100 += std::abs(f2->step( 0.0));
      tot100 += std::abs(f2->step( 0.0));
      tot100 += std::abs(f2->step( 0.0));
      tot100 += std::abs(f2->step( 0.0));
      
      if (tot/tot100 >= 0.999) break;
   }
   // Now find the 50% point
   tot50= tot100/2;
   f1= std::make_unique<RunBuf<double> >(run);
   tot= std::abs(f1->step(1.0));
   for (cnt= 0; tot < tot50; cnt++) 
      tot += std::abs(f1->step( 0.0));

   // Clean up, return
   return cnt;
}


//
//	Get the response and phase of a filter at the given frequency
//	(expressed as a proportion of the sampling rate, 0->0.5).
//	Phase is returned as a number from 0 to 1, representing a
//	phase between 0 and two-pi.
//

double 
Fid::response_pha(FidFilter *filt, double freq, double *phase) {
   auto top = std::complex<double>(1.0);
   auto bot = std::complex<double>(1.0);
   auto theta= freq * 2 * M_PI;
   auto zz  = std::polar<double>(1., theta);
   
   while (filt && filt->size()) {
      auto resp = evaluate(&filt->val[0], filt->size(), zz);
      if (filt->typ == 'I')
          bot *= resp;
      else if (filt->typ == 'F')
          top *= resp;
      else 
          error("Unknown filter type %d in response_pha()", filt->typ);
      filt= filt->next();
   }
   top /= bot;
   if (phase) {
      auto pha = std::arg(top)/(2*M_PI);
      if (pha < 0)
          pha += 1.0;
      *phase= pha;
   }
   return std::abs(top);
}

void *
Alloc(int size) {
   void *vp= calloc(1, size);
   if (!vp) error("Out of memory");
   return vp;
}
char *
strdupf(const char *fmt, ...) {
   va_list ap;
   char buf[1024], *rv;
   int len;
   va_start(ap, fmt);
   len= vsnprintf(buf, sizeof(buf), fmt, ap);
   if (len < 0 || static_cast<size_t>(len) >= sizeof(buf)-1) 
      error("strdupf exceeded buffer");
   rv= strdup(buf);
   if (!rv) error("Out of memory");
   return rv;
}
/*double  RunBuf::step(double val) {
   // Shift the whole internal array up one
   memmove(&buf[1], &buf[0], (run->n_buf-1)*sizeof(buf[0]));
   std::move(&buf[0],&buf[run->n_buf],&buf[1]);
   // Do IIR
   buf[0] = val - std::inner_product(run->iir.begin() + 1,run->iir.end(),buf.begin() + 1, 0);
   return   std::inner_product(run->fir.begin(),run->fir.end(),buf.begin(),0);
}*/
//
//	Create an instance of a filter, ready to run.  This returns a
//	void* handle, and a function to call to execute the filter.
//	Working buffers for the filter instances must be allocated
//	separately using run_newbuf().  This allows many
//	simultaneous instances of the filter to be run.  
//
//	The returned handle must be released using run_free().
//
/*Run::Run(const std::unique_ptr<FidFilter> &ff)
: Run(ff.get())
{
}
Run::Run(FidFilter *ff)
: filt(flatten(ff))
{
    ff = filt.get();
    if(!ff || ff->typ != 'I')
        throw std::bad_alloc();
    iir = ff->val;
    ff=ff->next();
    if(!ff || ff->typ != 'F')
        throw std::bad_alloc();
    fir = ff->val;
    ff=ff->next();
    if(ff && ff->size())
        throw std::bad_alloc();
    n_buf = std::max(fir.size(),iir.size());
}
//
//	Create a new instance of the given filter
//
RunBuf::RunBuf(Run *rr)
    :run(rr)
    ,buf(rr->n_buf)
{
}
RunBuf::RunBuf(const std::unique_ptr<Run> &rr)
    :RunBuf(rr.get())
{
}
void RunBuf::zap()
{
    std::fill(buf.begin(),buf.end(),0);
}*/
//
//	Delete the filter
//
void 
Fid::bessel(int order) {
    if (order > 10)
        error("Maximum Bessel order is 10");
    n_pol= order;
    auto poles = bessel_poles[order-1];
    auto a = 0;
    for(a = 0; a < order-1;){
        pol[a] = std::complex<double>(poles[a],poles[a+1]);
        poltyp[a]   = 2;
        poltyp[a+1] = 0;
        a += 2;
    }
    if (a < order) {
        pol[a]    = poles[a];
        poltyp[a] = 1;
    }
}
void 
Fid::butterworth(int order) {

    if (order > MAXPZ) 
        error("Maximum butterworth/chebyshev order is %d", MAXPZ);
    n_pol= order;
    auto a = 0;;
    for (a= 0; a<order-1;) {
        pol[a] = std::polar<double>(1.0, M_PI * ( 1.0 - (order - a - 1) * 0.5 / order ));
        poltyp[a]  = 2;
        poltyp[a+1]= 0;
        a += 2;
    }
    if (a < order) {
        poltyp[a]= 1;
        pol[a]= -1.0;
    }
}
void 
Fid::chebyshev(int order, double ripple) {

    butterworth(order);
    if (ripple >= 0.0) error("Chebyshev ripple in dB should be -ve");

    auto eps= std::sqrt(-1.0 + std::pow(10.0, -0.1 * ripple));
    auto y= std::asinh(1.0 / eps) / order;
    if (y <= 0.0) error("Internal error; chebyshev y-value <= 0.0: %g", y);
    auto sh= std::sinh(y);
    auto ch= std::cosh(y);
    for (auto a= 0; a<n_pol; ) {
        pol[a] = std::complex<double>(pol[a].real()*sh,pol[a].imag()*ch);
        if (poltyp[a] == 1){
            a++;
        }else {
            a+=2;
        }
    }
}
void 
Fid::lowpass(double freq) {
    // Adjust poles
    freq *= (2*M_PI);
    for (auto a= 0; a<n_pol; a++)
        pol[a] *= freq;
    // Add zeros
    n_zer= n_pol;
    for (auto a= 0; a<n_zer; a++) {
        zertyp[a]= 1;
        zer[a]   = -std::numeric_limits<double>::infinity();
    }
}
void 
Fid::highpass(double freq) {
    // Adjust poles
    freq *= (2*M_PI);
    for (auto a= 0; a<n_pol; ) {
        if(poltyp[a] == 1){
            pol[a] = freq/pol[a];
            a+=1;
        }else{
            pol[a] = freq/pol[a];
            a+=2;
        }
    }
    // Add zeros
    n_zer= n_pol;
    for (auto a= 0; a<n_zer; a++) {
        zertyp[a]= 1;
        zer[a]   = 0.0;
    }
}
void 
Fid::bandpass(double freq1, double freq2) {
    auto w0= (2*M_PI) * sqrt(freq1*freq2);
    auto bw= 0.5 * (2*M_PI) * (freq2-freq1);
    auto a = 0, b = 0;
    if (n_pol * 2 > MAXPZ) 
        error("Maximum order for bandpass filters is %d", MAXPZ/2);
    // Run through the list backwards, expanding as we go
    for (a= n_pol, b= n_pol*2; a>0; ) {
        // hba= pole * bw;
        // temp= c_sqrt(1.0 - square(w0 / hba));
        // pole1= hba * (1.0 + temp);
        // pole2= hba * (1.0 - temp);

        if (poltyp[a-1] == 1) {
            a--;
            b -= 2;
            poltyp[b]= 2;
            poltyp[b+1]= 0;
            auto hba= pol[a].real() * bw;
            auto tmp = std::sqrt(std::complex<double>(1.0 - ( w0 / hba ) * ( w0 / hba ),0.0));
            pol[b] = hba * ( 1.0 + tmp );
        } else {		// Assume poltyp[] data is valid
            a -= 2;
            b -= 4;
            poltyp[b]= 2;
            poltyp[b+1]= 0;
            poltyp[b+2]= 2;
            poltyp[b+3]= 0;
            auto hba = pol[a] * bw;
            auto tmp = std::sqrt(1.0 - ( w0/hba) * (w0/hba));
            pol[b]     = hba * ( 1.0 + tmp );
            pol[b + 2] = hba * ( 1.0 - tmp );
        } 
    }
    n_pol *= 2;
    n_zer= n_pol; 
    for (a= 0; a<n_zer; a++) {
        zertyp[a] = 1;
        zer[a]= (a<n_zer/2) ? 0.0 : -std::numeric_limits<double>::infinity();
    }
}
void 
Fid::bandstop(double freq1, double freq2) {
    auto w0= (2*M_PI) * std::sqrt(freq1*freq2);
    auto bw= 0.5 * (2*M_PI) * (freq2-freq1);
    int a, b;

    if (n_pol * 2 > MAXPZ) 
        error("Maximum order for bandstop filters is %d", MAXPZ/2);
    // Run through the list backwards, expanding as we go
    for (a= n_pol, b= n_pol*2; a>0; ) {
        // hba= pole * bw;
        // temp= c_sqrt(1.0 - square(w0 / hba));
        // pole1= hba * (1.0 + temp);
        // pole2= hba * (1.0 - temp);
        if (poltyp[a-1] == 1) {
            a -= 1;
            b -= 2;
            poltyp[b]= 2;
            poltyp[b+1]= 0;
            auto hba= pol[a].real() * bw;
            auto tmp = std::sqrt(std::complex<double>(1.0 - ( w0 / hba ) * ( w0 / hba ),0.0));
            pol[b] = hba * ( 1.0 + tmp );
        } else {		// Assume poltyp[] data is valid
            a -= 2;
            b -= 4;
            poltyp[b]  = 2;
            poltyp[b+1]= 0;
            poltyp[b+2]= 2;
            poltyp[b+3]= 0;
            auto hba = pol[a] * bw;
            auto tmp = std::sqrt(1.0 - ( w0/hba) * (w0/hba));
            pol[b]     = hba * ( 1.0 + tmp );
            pol[b + 2] = hba * ( 1.0 - tmp );
        } 
    }
    n_pol *= 2;
    // Add zeros
    n_zer= n_pol; 
    for (a= 0; a<n_zer; a+=2) {
        zertyp[a]  = 2;
        zertyp[a+1]= 0;
        zer[a]     = std::complex<double>(0,w0);
    }
}
void 
Fid::s2z_bilinear() {
    for (auto a= 0; a<n_pol; ) {
        // Calculate (2 + val) / (2 - val)
        if(poltyp[a] == 1) {
            if(pol[a] == std::complex<double>(-std::numeric_limits<double>::infinity()))
                pol[a] = -1.0;
            else
                pol[a] = (2.0 + pol[a]) / ( 2.0 - pol[a]);
            a++;
        }else{
            pol[a] = (2.0 + pol[a]) / ( 2.0 - pol[a]);
            a += 2;
        }
    }
    for (auto a= 0; a<n_zer; ) {
        // Calculate (2 + val) / (2 - val)
        if(zertyp[a] == 1) {
            if(zer[a] == std::complex<double>(-std::numeric_limits<double>::infinity()))
                zer[a] = -1;
            else
                zer[a] = (2.0 + zer[a]) / ( 2.0 - zer[a]);
            a += 1;
        }else{
            zer[a] = (2.0 + zer[a]) / ( 2.0 - zer[a]);
            a += 2;
        }
    }
}
    //
    //	Convert S to Z using matched-Z transform
    //
        
void 
Fid::s2z_matchedZ() {
    for (auto a= 0; a<n_pol; ) {
        // Calculate cexp(val)
        if (poltyp[a] == 1) {
            pol[a] = std::exp(pol[a]);
            a++;
        } else {
            pol[a] = std::exp(pol[a]);
            a += 2;
        }
    }
    for (auto a= 0; a<n_zer; ) {
        // Calculate cexp(val)
        zer[a] = std::exp(zer[a]);
        if(zertyp[a] == 1)
            a++;
        else
            a += 2;
    }
}
//
//	Generate a FidFilter for the current set of poles and zeros.
//	The given gain is inserted at the start of the FidFilter as a
//	one-coefficient FIR filter.  This is positioned to be easily
//	adjusted later to correct the filter gain.
//
//	'cbm' should be a bitmap indicating which FIR coefficients are
//	constants for this filter type.  Normal values are ~0 for all
//	constant, or 0 for none constant, or some other bitmask for a
//	mixture.  Filters generated with lowpass(), highpass() and
//	bandpass() above should pass ~0, but bandstop() requires 0x5.
//
//	This routine requires that any lone real poles/zeros are at
//	the end of the list.  All other poles/zeros are handled in
//	pairs (whether pairs of real poles/zeros, or conjugate pairs).
//

FidFilter*
Fid::z2fidfilter(double gain, int cbm) {
    int a;
    auto rv = std::make_unique<FidFilter>();
    auto ff = rv.get();
    auto n_head= 1 + n_pol + n_zer;	 // Worst case: gain + 2-element IIR/FIR
    ff->typ= 'F';
    ff->resize(1);
    ff->val[0]= gain;
    ff=ff->grow();
    // Output as much as possible as 2x2 IIR/FIR filters
    for (a= 0; a <= n_pol-2 && a <= n_zer-2; a += 2) {
        // Look for a pair of values for an IIR
        if (poltyp[a] == 1 && poltyp[a+1] == 1) {
        // Two real values
            ff->typ= 'I';
            ff->resize(3);
            ff->val[0]= 1;
            ff->val[1]= -(pol[a] + pol[a+1]).real();
            ff->val[2]=  (pol[a].real() * pol[a+1].real());
            ff= ff->grow();
        } else if (poltyp[a] == 2) {
        // A complex value and its conjugate pair
            ff->typ= 'I';
            ff->resize(3);
            ff->val[0]= 1;
            ff->val[1]= -2 * (pol[a].real());
            ff->val[2]=  std::norm(pol[a]);
            ff= ff->grow(); 
        } else error("Internal error -- bad poltyp[] values for z2fidfilter()");	
        // Look for a pair of values for an FIR
        if (zertyp[a] == 1 && zertyp[a+1] == 1) {
            // Two real values
            // Skip if constant and 0/0
            if (!cbm || zer[a] != 0.0 || zer[a+1] != 0.0) {
                ff->typ= 'F';
                ff->cbm= cbm;
                ff->resize(3);
                ff->val[0]= 1;
                ff->val[1]= -(zer[a].real() + zer[a+1].real());
                ff->val[2]=  (zer[a].real() * zer[a+1].real());
                ff= ff->grow(); 
            }
        } else if (zertyp[a] == 2) {
            // A complex value and its conjugate pair
            // Skip if constant and 0/0
            if (!cbm || zer[a] != 0.0 || zer[a+1] != 0.0) {
                ff->typ= 'F';
                ff->cbm= cbm;
                ff->resize(3);
                ff->val[0]= 1;
                ff->val[1]= - 2 * (zer[a].real());
                ff->val[2]=  std::norm(zer[a]);
                ff= ff->grow(); 
            }
        } else error("Internal error -- bad zertyp[] values");	
    }
    // Clear up any remaining bits and pieces.  Should only be a 1x1
    // IIR/FIR.
    if (n_pol-a == 0 && n_zer-a == 0) 
        ;
    else if (n_pol-a == 1 && n_zer-a == 1) {
        if (poltyp[a] != 1 || zertyp[a] != 1) 
            error("Internal error; bad poltyp or zertyp for final pole/zero");
        ff->typ= 'I';
        ff->resize(2);
        ff->val[0]= 1;
        ff->val[1]= -pol[a].real();
        ff= ff->grow();
        // Skip FIR if it is constant and zero
        if (!cbm || zer[a].real()) {
            ff->typ= 'F';
            ff->cbm= cbm;
            ff->resize(2);
            ff->val[0]= 1;
            ff->val[1]= -zer[a].real();
            ff= ff->grow();
        }
    } else 
        error("Internal error: unexpected poles/zeros at end of list");
    // End of list
    ff->clear();
    return rv.release();
}

double 
Fid::search_peak(FidFilter *ff, double f0, double f3) {
    // Binary search, modified, taking two intermediate points.  Do 20
    // subdivisions, which should give 1/2^20 == 1e-6 accuracy compared
    // to original range.
    for (auto a= 0; a<20; a++) {
        auto f1 = 0.51 * f0 + 0.49 * f3;
        auto f2= 0.49 * f0 + 0.51 * f3;
        if (f1 == f2)
            break;		// We're hitting FP limit
        auto r1 = response(ff, f1);
        auto r2 = response(ff, f2);
        if (r1 > r2)	// Peak is either to the left, or between f1/f2
            f3 = f2;
        else	 	// Peak is either to the right, or between f1/f2
            f0= f1;
    }
    return (f0+f3)*0.5;
}
FidFilter*
Fid::stack_filter(int order, int n_head, int n_val, ...) {
    auto rv = std::make_unique<FidFilter>();
    va_list ap;
    int a, b;
    if (order == 0)
        return rv.release();
    // Copy from ap
    va_start(ap, n_val);
    auto p = rv.get();
    for (a= 0; a<n_head; a++) {
        p->typ= va_arg(ap, int);
        p->cbm= va_arg(ap, int);
        p->resize(va_arg(ap, int));
        for (b= 0; b<p->size(); b++) 
            p->val[b]= va_arg(ap, double);
        p = p->grow();
    }
    order--;
    if(order > 0) {
    auto wrk = std::make_unique<FidFilter>();
    auto nxt = wrk.get();
    while(order-- > 0) {
            p = rv.get();
            while(p) {
                nxt->val = p->val;
                nxt->cbm = p->cbm;
                nxt->typ = p->typ;
                nxt = nxt->grow();
                p   = p->next();
            }
        }
        rv->back().m_next.swap(wrk);
    }
   return rv.release();
}
FidFilter *
Fid::design(const char *spec, double rate, double freq0, double freq1, int f_adj, char **descp) {
   auto rv = std::unique_ptr<FidFilter>();
   Spec sp;
   double f0, f1;
   char *err;

   // Parse the filter-spec
   sp.spec = spec;
   sp.in_f0= freq0;
   sp.in_f1= freq1;
   sp.in_adj= f_adj;
   err= parse_spec(&sp);
   if (err)
       error("%s", err);
   f0= sp.f0;
   f1= sp.f1;
   // Adjust frequencies to range 0-0.5, and check them
   f0 /= rate;
   if (f0 > 0.5)
       error("Frequency of %gHz out of range with sampling rate of %gHz", f0*rate, rate);
   f1 /= rate;
   if (f1 > 0.5)
       error("Frequency of %gHz out of range with sampling rate of %gHz", f1*rate, rate);
   // Okay we now have a successful spec-match to filter[sp.fi], and sp.n_arg
   // args are now in sp.argarr[]

   // Generate the filter
   if (!sp.adj)
      rv.reset((filter[sp.fi].rout)(this,rate, f0, f1, sp.order, sp.n_arg, sp.argarr));
   else if (strstr(filter[sp.fi].fmt, "#R"))
      rv.reset(auto_adjust_dual(&sp, rate, f0, f1));
   else 
      rv.reset( auto_adjust_single(&sp, rate, f0));
   // Generate a long description if required
   if (descp) {
      auto fmt= filter[sp.fi].txt;
      auto max = strlen(fmt) + 60 + sp.n_arg * 20;
      auto desc= (char*)Alloc(max);
      auto p   = desc;
      char ch;
      auto arg= sp.argarr;
      auto n_arg= sp.n_arg;
      while ((ch= *fmt++)) {
        if (ch != '#') {
            *p++= ch;
            continue;
        }
        switch (*fmt++) {
        case 'O':
            p += sprintf(p, "%d", sp.order);
            break;
        case 'F':
            p += sprintf(p, "%g", f0*rate);
            break;
        case 'R':
            p += sprintf(p, "%g-%g", f0*rate, f1*rate);
            break;
        case 'V':
            if (n_arg <= 0) 
            error("Internal error -- disagreement between filter short-spec\n"
                " and long-description over number of arguments");
            n_arg--;
            p += sprintf(p, "%g", *arg++);
            break;
        default:
            error("Internal error: unknown format in long description: #%c", fmt[-1]);
        }
      }
      *p++= 0;
      if (p-desc >= max)
          error("Internal error: exceeded estimated description buffer");
      *descp= desc;
   }
   return rv.release();
}
FidFilter*
Fid::auto_adjust_single(Fid::Spec *sp, double rate, double f0) {
   double a0, a1, a2;
   auto design = filter[sp->fi].rout;
   auto rv= std::unique_ptr<FidFilter>();
   double resp;
   double r0, r2;
   int incr;		// Increasing (1) or decreasing (0)
   int a;

#define DESIGN(aa) ((design)(this,rate, aa, aa, sp->order, sp->n_arg, sp->argarr))
#define TEST(aa) do{  rv.reset(DESIGN(aa)); resp= response(rv.get(), f0); }while(0)

   // Try and establish a range within which we can find the point
   a0= f0; TEST(a0); r0= resp;
   for (a= 2; 1; a*=2) {
      a2= f0/a; TEST(a2); r2= resp;
      if ((r0 < M301DB) != (r2 < M301DB)) break;
      a2= 0.5-((0.5-f0)/a); TEST(a2); r2= resp;
      if ((r0 < M301DB) != (r2 < M301DB)) break;
      if (a == 32) 	// No success
	 error("auto_adjust_single internal error -- can't establish enclosing range");
   }
   incr= r2 > r0;
   if (a0 > a2) { 
      a1= a0; a0= a2; a2= a1;
      incr= !incr;
   }
   // Binary search
   while (1) {
      a1= 0.5 * (a0 + a2);
      if (a1 == a0 || a1 == a2) break;		// Limit of double, sanity check
      TEST(a1);
      if (resp >= 0.9999995 * M301DB && resp < 1.0000005 * M301DB) break;
      if (incr == (resp > M301DB))
	 a2= a1;
      else 
	 a0= a1;
   }
#undef TEST
#undef DESIGN
   return rv.release();
}


FidFilter *
Fid::auto_adjust_dual(Fid::Spec *sp, double rate, double f0, double f1) {
   auto mid= 0.5 * (f0+f1);
   auto wid= 0.5 * std::abs(f1-f0);
   auto design = filter[sp->fi].rout;
   auto rv = std::unique_ptr<FidFilter>();
   auto bpass= -1;
   double delta;
   double mid0, mid1;
   double wid0, wid1;
   double r0, r1, err0, err1;
   double perr;
   auto cnt = 0;
   auto cnt_design= 0;

#define DESIGN(mm,ww) do{\
   rv.reset( (design)(this,rate, mm-ww, mm+ww, sp->order, sp->n_arg, sp->argarr)); \
   r0= response(rv.get(), f0); r1= response(rv.get(), f1); \
   err0= std::abs(M301DB-r0); err1= std::abs(M301DB-r1); cnt_design++; }while(false)

#define INC_WID ((r0+r1 < 1.0) == bpass)
#define INC_MID ((r0 > r1) == bpass)
#define MATCH (err0 < 0.000000499 && err1 < 0.000000499)
#define PERR (err0+err1)

   DESIGN(mid, wid);
   bpass= (response(rv.get(), 0) < 0.5);
   delta= wid * 0.5;
   
   // Try delta changes until we get there
   for (cnt= 0; 1; cnt++, delta *= 0.51) {
      DESIGN(mid, wid);		// I know -- this is redundant
      perr= PERR;

      mid0= mid;
      wid0= wid;
      mid1= mid + (INC_MID ? delta : -delta);
      wid1= wid + (INC_WID ? delta : -delta);
      
      if (mid0 - wid1 > 0.0 && mid0 + wid1 < 0.5) {
        DESIGN(mid0, wid1);
        if (MATCH) break;
        if (PERR < perr) { perr= PERR; mid= mid0; wid= wid1; }
      }
      if (mid1 - wid0 > 0.0 && mid1 + wid0 < 0.5) {
        DESIGN(mid1, wid0);
        if (MATCH) break;
        if (PERR < perr) { perr= PERR; mid= mid1; wid= wid0; }
      }
      if (mid1 - wid1 > 0.0 && mid1 + wid1 < 0.5) {
        DESIGN(mid1, wid1);
        if (MATCH) break;
        if (PERR < perr) { perr= PERR; mid= mid1; wid= wid1; }
      }
      if (cnt > 1000)
        error("auto_adjust_dual -- design not converging");
   }
#undef INC_WID
#undef INC_MID
#undef MATCH
#undef PERR
#undef DESIGN
   return rv.release();
}

double 
Fid::design_coef(double *coef, int n_coef, const char *spec, double rate, 
		double freq0, double freq1, int adj) {
   auto filt = std::unique_ptr<FidFilter>(design(spec, rate, freq0, freq1, adj, 0));
   auto ff = filt.get();
   auto a = 0, len = 0;
   auto cnt = 0;
   auto gain= 1.0;
   double *iir, *fir, iir_adj = 0;
   static double const_one= 1;
   int n_iir, n_fir;
   int iir_cbm, fir_cbm;
   while (ff  && ff->typ) {
      if (ff->typ == 'F' && ff->size()== 1) {
        gain *= ff->val[0];
        ff= ff->next();
        continue;
      }
      if (ff->typ != 'I' && ff->typ != 'F')
        error("Fid::design_coef can't handle FidFilter type: %c", ff->typ);
      // Initialise to safe defaults
      iir= fir= &const_one;
      n_iir = n_fir= 1;
      iir_cbm = fir_cbm= ~0;
      // See if we have an IIR filter
      if (ff->typ == 'I') {
        iir= &ff->val[0];
        n_iir= ff->size();
        iir_cbm= ff->cbm;
        iir_adj= 1.0 / ff->val[0];
        ff = ff->next();
        gain *= iir_adj;
      }
      // See if we have an FIR filter
      if (ff->typ == 'F') {
        fir= &ff->val[0];
        n_fir= ff->size();
        fir_cbm= ff->cbm;
        ff = ff->next();
      }
      // Dump out all non-const coefficients in reverse order
      len = n_fir > n_iir ? n_fir : n_iir;
      for (a = len-1; a>=0; a--) {
        // Output IIR if present and non-const
        if (a < n_iir && a>0 && !(iir_cbm & (1<<(a<15?a:15)))) {
            if (cnt++ < n_coef)
                *coef++= iir_adj * iir[a];
        }
        // Output FIR if present and non-const
        if (a < n_fir && !(fir_cbm & (1<<(a<15?a:15)))) {
            if (cnt++ < n_coef)
                *coef++ = fir[a];
        }
      }
   }
   if (cnt != n_coef)
      error("Fid::design_coef called with the wrong number of coefficients.\n"
	    "  Given %d, expecting %d: (\"%s\",%g,%g,%g,%d)",
	    n_coef, cnt, spec, rate, freq0, freq1, adj);
   return gain;
}
//
//	Generate a combined filter -- merge all the IIR/FIR
//	sub-filters into a single IIR/FIR pair, and make sure the IIR
//	first coefficient is 1.0.
//
FidFilter * flatten(FidFilter *filt) {
   auto m_fir= 1;	// Maximum values
   auto m_iir= 1;
   auto n_fir = 0, n_iir = 0;	// Stored counts during convolution

   // Find the size of the output filter
   auto ff = filt;
   while (ff && ff->size()) {
      if (ff->typ == 'I')
        m_iir += ff->size()-1;
      else if (ff->typ == 'F')
        m_fir += ff->size()-1;
      else 
        error("Fid::flatten doesn't know about type %d", ff->typ);
      ff= ff->next();
   }
   auto rv = std::make_unique<FidFilter>();
   rv->typ= 'I';
   rv->resize(m_iir);
   auto iir= &rv->val[0];
   ff= rv->grow();
   ff->typ= 'F';
   ff->resize(m_fir);
   auto fir= &ff->val[0];

   iir[0]= 1.0; n_iir= 1;
   fir[0]= 1.0; n_fir= 1;
   // Do the convolution
   ff = filt;
   while (ff && ff->size()) {
      if (ff->typ == 'I') 
        n_iir= convolve(iir, n_iir, &ff->val[0], ff->size());
      else 
        n_fir= convolve(fir, n_fir, &ff->val[0], ff->size());
      ff= ff->next();
   }
   // Sanity check
   if (n_iir != m_iir || n_fir != m_fir) 
      error("Internal error in Fid::combine() -- array under/overflow");
   // Fix iir[0]
   auto adj= 1.0/iir[0];
   for (auto a= 0; a<n_iir; a++)
       iir[a] *= adj;
   for (auto a= 0; a<n_fir; a++)
       fir[a] *= adj;
   return rv.release();
}


char *
Fid::parse_spec(Spec *sp) {
   double *arg;
   int a;
   arg= sp->argarr;
   sp->n_arg= 0;
   sp->order= 0;
   sp->f0= 0;
   sp->f1= 0; 
   sp->adj= 0;
   sp->minlen= -1;
   sp->n_freq= 0;
   for (a= 0; 1; a++) {
      auto fmt= filter[a].fmt;
      auto p= sp->spec;
      char ch, *q;

      if (!fmt)
          return strdupf("Spec-string \"%s\" matches no known format", sp->spec);
      while (*p && (ch= *fmt++)) {
        if (ch != '#') {
            if (ch == *p++)
                continue;
            p= 0; break;
        }
        if (isalpha(*p)) {
            p= 0;
            break;
        }
        // Handling a format character
        switch (ch= *fmt++) {
        default:
            return strdupf("Internal error: Unknown format #%c in format: %s", 
                    fmt[-1], filter[a].fmt);
        case 'o':
        case 'O':
            sp->order= (int)strtol(p, &q, 10);
            if (p == q) {
            if (ch == 'O')
                goto bad;
            sp->order= 1;
            }
            if (sp->order <= 0) 
            return strdupf("Bad order %d in spec-string \"%s\"", sp->order, sp->spec);
            p= q; break;
        case 'V':
            sp->n_arg++; 
            *arg++= strtod(p, &q);
            if (p == q)
                goto bad; 
            p= q; break;
        case 'F':
            sp->minlen= p-1-sp->spec;
            sp->n_freq= 1;
            sp->adj= (p[0] == '=');
            if (sp->adj) p++;
            sp->f0= strtod(p, &q);
            sp->f1= 0;
            if (p == q)
                goto bad; 
            p= q; break;
        case 'R':
            sp->minlen= p-1-sp->spec;
            sp->n_freq= 2;
            sp->adj= (p[0] == '=');
            if (sp->adj) p++;
            sp->f0= strtod(p, &q);
            if (p == q)
                goto bad; 
            p= q;
            if (*p++ != '-')
                goto bad;
            sp->f1= strtod(p, &q);
            if (p == q)
                goto bad; 
            if (sp->f0 > sp->f1) 
            return strdupf("Backwards frequency range in spec-string \"%s\"", sp->spec);
            p= q; break;
        }
      }
      if (p == 0) continue;
      if (fmt[0] == '/' && fmt[1] == '#' && fmt[2] == 'F') {
        sp->minlen= p-sp->spec;
        sp->n_freq= 1;
        if (sp->in_f0 < 0.0) 
            return strdupf("Frequency omitted from filter-spec, and no default provided");
        sp->f0= sp->in_f0;
        sp->f1= 0;
        sp->adj= sp->in_adj;
        fmt += 3;
      } else if (fmt[0] == '/' && fmt[1] == '#' && fmt[2] == 'R') {
        sp->minlen= p-sp->spec;
        sp->n_freq= 2;
        if (sp->in_f0 < 0.0 || sp->in_f1 < 0.0)
            return strdupf("Frequency omitted from filter-spec, and no default provided");
        sp->f0= sp->in_f0;
        sp->f1= sp->in_f1;
        sp->adj= sp->in_adj;
        fmt += 3;
      }
      // Check for trailing unmatched format characters
      if (*fmt) {
      bad:
        return strdupf("Bad match of spec-string \"%s\" to format \"%s\"", sp->spec, filter[a].fmt);
      }
      if (sp->n_arg > Spec::MAXARG) 
        return strdupf("Internal error -- maximum arguments exceeded");
      // Set the minlen to the whole string if unset
      if (sp->minlen < 0)
          sp->minlen= p-sp->spec;
      // Save values, return
      sp->fi= a;
      return 0;
   }
   return 0;
}
void 
Fid::rewrite_spec(const char *spec, double freq0, double freq1, int adj,
		 char **spec1p, 
		 char **spec2p, double *freq0p, double *freq1p, int *adjp) {
   Spec sp;
   char *err;
   sp.spec= spec;
   sp.in_f0= freq0;
   sp.in_f1= freq1;
   sp.in_adj= adj;
   err= parse_spec(&sp);
   if (err)
       error("%s", err);
   if (spec1p) {
      char buf[128];
      int len;
      char *rv;
      switch (sp.n_freq) {
        case 1: sprintf(buf, "/%s%.15g", sp.adj ? "=" : "", sp.f0); break;
        case 2: sprintf(buf, "/%s%.15g-%.15g", sp.adj ? "=" : "", sp.f0, sp.f1); break;
        default: buf[0]= 0;
      }
      len = strlen(buf);
      rv  = (char*)Alloc(sp.minlen + len + 1);
      memcpy(rv, spec, sp.minlen);
      strcpy(rv+sp.minlen, buf);
      *spec1p= rv;
   }
   if (spec2p) {
      auto rv= (char*)Alloc(sp.minlen + 1);
      memcpy(rv, spec, sp.minlen);
      *spec2p= rv;
      *freq0p= sp.f0;
      *freq1p= sp.f1;
      *adjp= sp.adj;
   }
}
FidFilter *
Fid::cv_array(double *arr) {
   auto n_head= 0;
   auto n_val= 0;
   // Scan through for sizes
   for (auto dp = arr; *dp; ) {
      auto typ = (int)(*dp++);
      if (typ != 'F' && typ != 'I') 
        error("Bad type in array passed to Fid::cv_array: %g", dp[-1]);
      auto len = (int)(*dp++);
      if (len < 1)
        error("Bad length in array passed to Fid::cv_array: %g", dp[-1]);
      n_head++;
      n_val += len;
      dp += len;
   }
   auto rv = std::make_unique<FidFilter>();
   auto ff = rv.get();
   // Scan through to fill in FidFilter
   for (auto dp = arr; *dp; ) {
      auto typ = (int)(*dp++);
      auto len = (int)(*dp++);

      ff->typ = typ;
      ff->resize(len);
      memcpy(&ff->val[0], dp, len * sizeof(double));
      dp += len;
      ff = ff->grow();
   }
   // Final element already zero'd thanks to allocation
   return rv.release();
}
//
//	Create a single filter from the given list of filters in
//	order.  If 'freeme' is set, then all the listed filters are
//	free'd once read; otherwise they are left untouched.  The
//	newly allocated resultant filter is returned, which should be
//	released with free() when finished with.
//      

FidFilter *
Fid::cat(int freeme, ...) {
   va_list ap;
   auto rv = std::make_unique<FidFilter>();
   FidFilter *ff, *ff0;
   ff = rv.get();
   // Find the memory required to store the combined filter
   va_start(ap, freeme);
   while ((ff0= va_arg(ap, FidFilter*))) {
      for (; ff0; ff0= ff0->next()) {
        ff->val = ff0->val;
        ff->typ = ff0->typ;
        ff = ff->grow();
      }
	 ;
     if(freeme)
         delete ff0;
   }
   va_end(ap);
   // Final element already zero'd
   return rv.release();
}
char *
Fid::parse(double rate, char **pp, FidFilter **ffp) {
   char buf[128];
   char *p= *pp, *rew;
#undef INIT_LEN
   auto rv = std::make_unique<FidFilter>();
   auto curr = rv.get();
   int typ= -1;		// First time through
   double val;
   char dmy;

#define ERR(ptr, msg) { *pp= ptr; *ffp= 0; return msg; }
   while (1) {
      rew= p;
      if (!grabWord(&p, buf, sizeof(buf))) {
            if (*p)
                ERR(p, "Filter element unexpectedly long -- syntax error?");
            buf[0]= 0;
      }
    if (!buf[0] || !buf[1]) switch (buf[0]) {
        default:
            break;
        case 0:
        case ',':
        case ';':
        case ')':
        case ']':
        case '}':
            // End of filter, return it
            curr->typ= 0;
            curr->resize(0);
            *pp = buf[0] ? (p-1) : p;
            *ffp= rv.release();
            return 0;
        case '/':
            if (typ > 0)
                ERR(rew, "Filter syntax error; unexpected '/'");
            typ= 'I';
            continue;
        case 'x':
            if (typ > 0)
                ERR(rew, "Filter syntax error; unexpected 'x'");
            typ= 'F';
            continue;
    }
      if (typ < 0)
          typ= 'F';		// Assume 'x' if missing
      if (!typ)
          ERR(p, "Expecting a 'x' or '/' before this");
      if (1 != sscanf(buf, "%lf %c", &val, &dmy)) {
        // Must be a predefined filter
        FidFilter *ff;
        Spec sp;
        double f0, f1;
        char *err;
        if (typ != 'F')
            ERR(rew, "Predefined filters cannot be used with '/'");
        // Parse the filter-spec
        memset(&sp, 0, sizeof(sp));
        sp.spec= buf;
        sp.in_f0= sp.in_f1= -1;
        if ((err= parse_spec(&sp)))
            ERR(rew, err);
        f0= sp.f0;
        f1= sp.f1;
        // Adjust frequencies to range 0-0.5, and check them
        f0 /= rate;
        if (f0 > 0.5) ERR(rew, strdupf("Frequency of %gHz out of range with "
                        "sampling rate of %gHz", f0*rate, rate));
        f1 /= rate;
        if (f1 > 0.5) ERR(rew, strdupf("Frequency of %gHz out of range with "
                        "sampling rate of %gHz", f1*rate, rate));
        
        // Okay we now have a successful spec-match to filter[sp.fi], and sp.n_arg
        // args are now in sp.argarr[]
        
        // Generate the filter
        if (!sp.adj)
            ff = (filter[sp.fi].rout)(this,rate, f0, f1, sp.order, sp.n_arg, sp.argarr);
        else if (strstr(filter[sp.fi].fmt, "#R"))
            ff = auto_adjust_dual(&sp, rate, f0, f1);
        else 
            ff = auto_adjust_single(&sp, rate, f0);
        // Append it to our FidFilter to return
        rv->back().m_next.reset(ff);
        curr = ff->back().grow();
        typ= 0;
        continue;
      }
      // Must be a list of coefficients
      curr->typ= typ;
      curr->resize(1);
      curr->val[0] = val;
      // See how many more coefficients we can pick up
      while (1) {
        rew= p;
        if (!grabWord(&p, buf, sizeof(buf))) {
            if (*p)
                ERR(p, strdupf("Filter element unexpectedly long -- syntax error?"));
            buf[0]= 0;
        }
        if (1 != sscanf(buf, "%lf %c", &val, &dmy)) {
            p = rew;
            break;
        }
        curr->val.push_back(val);
        }
      curr = curr->grow();
      typ= 0;
      continue;
   }
#undef INCBUF
#undef ERR
   return strdupf("Internal error, shouldn't reach here");
}
void 
Fid::expand_spec(char *buf, char *bufend, const char *str) {
   int ch;
   char *p= buf;
   while ((ch= *str++)) {
      if (p + 10 >= bufend) 
        error("Buffer overflow in fidlib expand_spec()");
      if (ch == '#') {
        switch (*str++) {
            case 'o': p += sprintf(p, "<optional-order>"); break;
            case 'O': p += sprintf(p, "<order>"); break;
            case 'F': p += sprintf(p, "<freq>"); break;
            case 'R': p += sprintf(p, "<range>"); break;
            case 'V': p += sprintf(p, "<value>"); break;
            default: p += sprintf(p, "<%c>", str[-1]); break;
        }
      } else {
        *p++= ch;
      }
   }
   *p= 0;
}
void 
Fid::list_filters(FILE *out) {
   int a;
   for (a= 0; filter[a].fmt; a++) {
      char buf[4096];
      expand_spec(buf, buf+sizeof(buf), filter[a].fmt);
      fprintf(out, "%s\n    ", buf);
      expand_spec(buf, buf+sizeof(buf), filter[a].txt);
      fprintf(out, "%s\n", buf);
   }
}
//
//	List all the known filters to the given buffer; the buffer is
//	NUL-terminated; returns 1 okay, 0 not enough space
//
int 
Fid::list_filters_buf(char *buf, char *bufend) {
   int cnt;
   char tmp[4096];
   for (auto a= 0; filter[a].fmt; a++) {
      expand_spec(tmp, tmp+sizeof(tmp), filter[a].fmt);
      buf += (cnt= snprintf(buf, bufend-buf, "%s\n    ", tmp));
      if (cnt < 0 || buf >= bufend) return 0;
      expand_spec(tmp, tmp+sizeof(tmp), filter[a].txt);
      buf += (cnt= snprintf(buf, bufend-buf, "%s\n", tmp));
      if (cnt < 0 || buf >= bufend) return 0;
   }
   return 1;
}
int
convolve(double *dst, int n_dst, double *src, int n_src) {
   int len= n_dst + n_src - 1;
   for (auto a= len-1; a>=0; a--) {
      auto val= 0.0;
      for (auto b= 0; b<n_src; b++)
         if (a-b >= 0 && a-b < n_dst)
            val += src[b] * dst[a-b];
      dst[a]= val;
   }
   return len;
}
const char *
Fid::version()
{
    return VERSION;
}
