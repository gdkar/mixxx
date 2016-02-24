//
//	Fidlib digital filter designer code
//	-----------------------------------
//
//        Copyright (c) 2002-2004 Jim Peters <http://uazu.net/>.  This
//        file is released under the GNU Lesser General Public License
//        (LGPL) version 2.1 as published by the Free Software
//        Foundation.  See the file COPYING_LIB for details, or visit
//        <http://www.fsf.org/licenses/licenses.html>.
//
//	The code in this file was written to go with the Fiview app
//	(http://uazu.net/fiview/), but it may be used as a library for
//	other applications.  The idea behind this library is to allow
//	filters to be designed at run-time, which gives much greater
//	flexibility to filtering applications.
//
//	This file depends on the fidmkf.h file which provides the
//	filter types from Tony Fisher's 'mkfilter' package.  See that
//	file for references and links used there.
//
//
//	Here are some of the sources I used whilst writing this code:
//
//	Robert Bristow-Johnson's EQ cookbook formulae:
//	  http://www.harmony-central.com/Computer/Programming/Audio-EQ-Cookbook.txt
//	
_Pragma("once")
#include <limits>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <climits>

typedef struct FidFilter FidFilter;
struct FidFilter {
   short typ;		// Type of filter element 'I' IIR, 'F' FIR, or 0 for end of list
   short cbm;		// Constant bitmap.  Bits 0..14, if set, indicate that val[0..14]
   			//   is a constant across changes in frequency for this filter type
   			//   Bit 15, if set, indicates that val[15..inf] are constant.
   int len;		// Number of doubles stored in val[], or 0 for end of list
   double val[1];
};
#include <cmath>
inline double 
prewarp(double val) {
   return tan(val * M_PI) / M_PI;
}
// Lets you write: for (; ff->typ; ff= FFNEXT(ff)) { ... }
#define FFNEXT(ff) ((FidFilter*)((ff)->val + (ff)->len))

// Size of a sub-filter with 'cnt' double values attached
#define FFSIZE(cnt) (sizeof(FidFilter) + ((cnt)-1)*sizeof(double))

// Size required for the memory chunk to contain the given number
// headers and values, plus termination
#define FFCSIZE(n_head,n_val) ((sizeof(FidFilter)-sizeof(double))*((n_head)+1) + sizeof(double)*(n_val))

// Allocate the chunk of memory to hold a list of FidFilters, with
// n_head FidFilters and n_val total double values in the whole list.
// Includes space for the final termination, and zeros the memory.
#define FFALLOC(n_head,n_val) (FidFilter*)Alloc(FFCSIZE(n_head, n_val))

// These are so you can use easier names to refer to running filters
typedef void FidRun;
typedef double (FidFunc)(void*, double);
struct Run;
struct RunBuf;
//
//	Filter specification string
//	---------------------------
//
//	The filter specification string can be used to completely
//	specify the filter, or it can be used with the frequency or
//	frequency range missing, in which case default values are
//	picked up from values passed directly to the routine.
//	
//	The spec consists of a series of letters usually followed by
//	the order of the filter and then by any other parameters
//	required, preceded by slashes.  For example:
//
//	  LpBu4/20.4	Lowpass butterworth, 4th order, -3.01dB at 20.4Hz
//	  BpBu2/3-4	Bandpass butterworth, 2nd order, from 3 to 4Hz
//	  BpBu2/=3-4	Same filter, but adjusted exactly to the range given
//	  BsRe/1000/10	Bandstop resonator, Q=1000, frequency 10Hz
//
//	The routines fid_design() or fid_parse() are used to convert
//	this spec-string into filter coefficients and a description
//	(if required).
//
//
//	Typical usage:
//	-------------
//
//	FidFilter *filt, *filt2;
//	char *desc;
//	FidRun *run;
//	FidFunc *funcp;
//	void *fbuf1, *fbuf2;
//	int delay;
//	void my_error_func(char *err);
//
//	// Design a filter, and optionally get its long description
//	filt= fid_design(spec, rate, freq0, freq1, adj, &desc);
//
//	// List all the possible filter types
//	fid_list_filters(stdout);
//	okay= fid_list_filters_buf(buf, buf+sizeof(buf));
//
//	// Calculate the response of the filter at a given frequency
//	// (frequency is given as a proportion of the sampling rate, in 
//	// the range 0 to 0.5).  If phase is returned, then this is 
//	// given in the range 0 to 1 (for 0 to 2*pi).
//	resp= fid_response(filt, freq);
//	resp= fid_response_pha(filt, freq, &phase);
//
//	// Estimate the signal delay caused by a particular filter, in samples
//	delay= fid_calc_delay(filt);
//	
//	// Run a given filter (this will do JIT filter compilation if this is
//	// implemented for this processor / OS)
//	run= fid_run_new(filt, &funcp);
//	fbuf1= fid_run_newbuf(run);
//	fbuf2= fid_run_newbuf(run);
//	while (...) {
//	   out_1= funcp(fbuf1, in_1);
//	   out_2= funcp(fbuf2, in_2);
//	   if (restart_required) fid_run_zapbuf(fbuf1);
//	   ...
//	}
//	fid_run_freebuf(fbuf2);
//	fid_run_freebuf(fbuf1);
//	fid_run_free(run);
//
//	// If you need to allocate your own buffers separately for some 
//	// reason, then do it this way:
//	run= fid_run_new(filt, &funcp);
//	len= fid_run_bufsize(run);
//	fbuf1= Alloc(len); fid_run_initbuf(run, fbuf1);
//	fbuf2= Alloc(len); fid_run_initbuf(run, fbuf2);
//	while (...) {
//	   out_1= funcp(fbuf1, in_1);
//	   out_2= funcp(fbuf2, in_2);
//	   if (restart_required) fid_run_zapbuf(fbuf1);
//	   ...
//	}
//	free(fbuf2);
//	free(fbuf1);
//	fid_run_free(run);
//
//	// Convert an arbitrary filter into a new filter which is a single 
//	// IIR/FIR pair.  This is done by convolving the coefficients.  This 
//	// flattened filter will give the same result, in theory.  However, 
//	// in practice this will be less accurate, especially in cases where 
//	// the limits of the floating point format are being reached (e.g. 
//	// subtracting numbers with small highly significant differences).
//	// The routine also ensures that the IIR first coefficient is 1.0.
//	filt2= fid_flatten(filt);
//	free(filt);
//
//	// Parse an entire filter-spec string possibly containing several FIR,
//	// IIR and predefined filters and return it as a FidFilter at the given 
//	// location.  Stops at the first ,; or unmatched )]} character, or the end
//	// of the string.  Returns a strdup'd error string on error, or else 0.
//	err= fid_parse(double rate, char **pp, FidFilter **ffp);
//
//	// Set up your own fatal-error handler (default is to dump a message 
//	// to STDERR and exit on fatal conditions)
//	fid_set_error_handler(&my_error_func);
//
//	// Get the version number of the library as a string (e.g. "1.0.0")
//	txt= fid_version();
//
//	// Design a filter and reduce it to a list of all the non-const 
//	// coefficients, which is returned in the given double[].  The number
//	// of coefficients expected must be provided (as a check).
//	#define N_COEF <whatever>
//	double coef[N_COEF], gain;
//	gain= fid_design_coef(coef, N_COEF, spec, rate, freq0, freq1, adj);
//
//	// Rewrite a filter spec in a full and/or separated-out form
//	char *full, *min;
//	double minf0, minf1;
//	int minadj;
//	fid_rewrite_spec(spec, freq0, freq1, adj, &full, &min, &minf0, &minf1, &minadj);
//	...
//	free(full); free(min);
//
//	// Create a FidFilter based on coefficients provided in the
//	// given double array.  
//	static double array[]= { 'I', 3, 1.0, 0.55, 0.77, 'F', 3, 1, -2, 1, 0 };
//	filt= fid_cv_array(array);
//
//	// Join a number of filters into a single filter (and free them too, 
//	// if the first argument is 1)
//	filt= fid_cat(0, filt1, filt2, filt3, filt4, 0);
//
//

//
//	Format of returned filter
//	-------------------------
//
//	The filter returned is a single chunk of allocated memory in
//	which is stored a number of FidFilter instances.  Each
//	instance has variable length according to the coefficients
//	contained in it.  It is probably easier to think of this as a
//	stream of items in memory.  Each sub-filter starts with its
//	type as a short -- either 'I' for IIR filters, or 'F' for FIR
//	filters.  (Other types may be added later, e.g. AM modulation
//	elements, or whatever).  This is followed by a short bitmap
//	which indicates which of the coefficients are constants,
//	aiding code-generation.  Next comes the count of the following
//	coefficients, as an int.  (These header fields normally takes 8 
//	bytes, the same as a double, but this might depend on the 
//	platform).  Then follow the coefficients, as doubles.  The next 
//	sub-filter follows on straight after that.  The end of the list 
//	is marked by 8 zero bytes, meaning typ==0, cbm==0 and len==0.
//
//	The filter can be read with the aid of the FidFilter structure
//	(giving typ, cbm, len and val[] elements) and the FFNEXT() 
//	macro: using ff= FFNEXT(ff) steps to the next FidFilter 
//	structure along the chain.
//
//	Note that within the sub-filters, coefficients are listed in
//	the order that they apply to data, from current-sample
//	backwards in time, i.e. most recent first (so an FIR val[] of
//	0, 0, 1 represents a two-sample delay FIR filter).  IIR
//	filters are *not* necessarily adjusted so that their first
//	coefficient is 1.
//
//	Most filters have their gain pre-adjusted so that some
//	suitable part of the response is at gain==1.0.  However, this
//	depends on the filter type.
//

//
//	Check that a target macro has been set.  This macro selects
//	various fixes required on various platforms:
//
//	  T_LINUX  Linux, or probably any UNIX-like platform with GCC
//	  T_MINGW  MinGW -- either building on Win32 or cross-compiling
//	  T_MSVC   Microsoft Visual C
//
//	(On MSVC, add "T_MSVC" to the preprocessor definitions in the
//	project settings, or add /D "T_MSVC" to the compiler
//	command-line.)
//


//
//	Select which method of filter execution is preferred.
//	RF_CMDLIST is recommended (and is the default).
//
//	  RF_COMBINED -- easy to understand code, lower accuracy
//	  RF_CMDLIST  -- faster pre-compiled code
//	  RF_JIT      -- fastest JIT run-time generated code (no longer supported)
//

#ifndef RF_COMBINED
#ifndef RF_CMDLIST

#define RF_COMBINED

#endif
#endif

//
//	Includes
//

#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <cmath>

///extern "C" FidFilter *mkfilter(char *, ...);

//
//	Target-specific fixes
//

// Macro for local inline routines that shouldn't be visible externally
// See Mixxx Bug #1179683

//
//	Support code
//

char *
strdupf(const char *fmt, ...) ;
void *
Alloc(int size);
#define ALLOC(type) ((type*)Alloc(sizeof(type)))
#define ALLOC_ARR(cnt, type) ((type*)Alloc((cnt) * sizeof(type)))

//
//	Get the response and phase of a filter at the given frequency
//	(expressed as a proportion of the sampling rate, 0->0.5).
//	Phase is returned as a number from 0 to 1, representing a
//	phase between 0 and two-pi.
//



//
//	Estimate the delay that a filter causes to the signal by
//	looking for the point at which 50% of the filter calculations
//	are complete.  This involves running test impulses through the
//	filter several times.  The estimated delay in samples is
//	returned.
//	
//	Delays longer than 8,000,000 samples are not handled well, as
//	the code drops out at this point rather than get stuck in an
//	endless loop.
//


//
//	'mkfilter'-derived code
//

struct Run {
   int magic;		// Magic: 0x64966325
   double *fir;         // FIR parameters
   int   n_fir;           // Number of FIR parameters
   double *iir;         // IIR parameters
   int   n_iir;           // Number of IIR parameters
   int   n_buf;           // Number of entries in buffer
   FidFilter *filt;	// Combined filter
} ;

struct RunBuf {
   Run *run;
   double buf[0];
};

//
//	Stack a number of identical filters, generating the required
//	FidFilter* return value
//


class Fid {
#define MAXPZ 64 
    int n_pol;		// Number of poles
    double pol[MAXPZ];	// Pole values (see above)
    char poltyp[MAXPZ];	// Pole value types: 1 real, 2 first of complex pair, 0 second
    int n_zer;		// Same for zeros ...
    double zer[MAXPZ];
    char zertyp[MAXPZ];	
    
    //
    //	Pre-warp a frequency
    //



    //
    //	Bessel poles; final one is a real value for odd numbers of
    //	poles
    //

    static  const double bessel_1[];
    static  const double bessel_2[];
    static  const double bessel_3[];
    static  const double bessel_4[];
    static  const double bessel_5[];
    static  const double bessel_6[];
    static  const double bessel_7[];
    static  const double bessel_8[];
    static  const double bessel_9[];
    static  const double bessel_10[];
    static  const double * const bessel_poles[];

    //
    //	Generate Bessel poles for the given order.
    //

    void 
    bessel(int order);
    //
    //	Generate Butterworth poles for the given order.  These are
    //	regularly-spaced points on the unit circle to the left of the
    //	real==0 line.
    //

    void 
    butterworth(int order);
    //
    //	Generate Chebyshev poles for the given order and ripple.
    //

    void 
    chebyshev(int order, double ripple);
    //
    //	Adjust raw poles to LP filter
    //

    void 
    lowpass(double freq);
    //
    //	Adjust raw poles to HP filter
    //
    void 
    highpass(double freq);
    //
    //	Adjust raw poles to BP filter.  The number of poles is
    //	doubled.
    //
    void 
    bandpass(double freq1, double freq2);
    //
    //	Adjust raw poles to BS filter.  The number of poles is
    //	doubled.
    //
    void 
    bandstop(double freq1, double freq2);
    //
    //	Convert list of poles+zeros from S to Z using bilinear
    //	transform
    //

    void 
    s2z_bilinear();
    //
    //	Convert S to Z using matched-Z transform
    //
        
    void 
    s2z_matchedZ();
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
    z2fidfilter(double gain, int cbm);

    //
    //	Setup poles/zeros for a band-pass resonator.  'qfact' gives
    //	the Q-factor; 0 is a special value indicating +infinity,
    //	giving an oscillator.
    //

    void 
    bandpass_res(double freq, double qfact);
    //
    //	Setup poles/zeros for a bandstop resonator
    //
    void 
    bandstop_res(double freq, double qfact);
    //
    //	Setup poles/zeros for an allpass resonator
    //
    void 
    allpass_res(double freq, double qfact);
    double 
    search_peak(FidFilter *ff, double f0, double f3);


    //
    //	Setup poles/zeros for a proportional-integral filter
    //
    //
    void 
    prop_integral(double freq);
    FidFilter*
    do_lowpass(int mz, double freq);

    FidFilter*
    do_highpass(int mz, double freq);
    FidFilter*
    do_bandpass(int mz, double f0, double f1);
    FidFilter*
    do_bandstop(int mz, double f0, double f1);

    int 
    calc_delay(FidFilter *filt);
    double 
    response_pha(FidFilter *filt, double freq, double *phase);

//
//	Get the response of a filter at the given frequency (expressed
//	as a proportion of the sampling rate, 0->0.5).
//
//	Code duplicate, as I didn't want the overhead of a function
//	call to fid_response_pha.  Almost every call in this routine
//	can be inlined.
//

double 
response(FidFilter *filt, double freq);
FidFilter*
stack_filter(int order, int n_head, int n_val, ...);

#define BL 0
#define MZ 1

//
//	Information passed to individual filter design routines:
//
//	  double* rout(double rate, double f0, double f1, 
//		       int order, int n_arg, double *arg);
//
//	'rate' is the sampling rate, or 1 if not set
//	'f0' and 'f1' give the frequency or frequency range as a 
//	 	proportion of the sampling rate
//	'order' is the order of the filter (the integer passed immediately 
//		after the name)
//	'n_arg' is the number of additional arguments for the filter
//	'arg' gives the additional argument values: arg[n]
//
//	Note that #O #o #F and #R are mapped to the f0/f1/order
//	arguments, and are not included in the arg[] array.
//
//	See the previous description for the required meaning of the
//	return value FidFilter list.
//	

//
//	Filter design routines and supporting code
//

FidFilter*
des_bpre(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bsre(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_apre(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_pi(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_piz(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lpbe(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_hpbe(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bpbe(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bsbe(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lpbez(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_hpbez(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bpbez(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bsbez(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*	// Butterworth-Bessel cross
des_lpbube(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lpbu(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_hpbu(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bpbu(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bsbu(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lpbuz(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_hpbuz(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bpbuz(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bsbuz(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lpch(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_hpch(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bpch(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bsch(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lpchz(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_hpchz(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bpchz(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bschz(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lpbq(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_hpbq(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bpbq(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_bsbq(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_apbq(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_pkbq(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lsbq(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_hsbq(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lpbl(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lphm(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lphn(double rate, double f0, double f1, int order, int n_arg, double *arg);
FidFilter*
des_lpba(double rate, double f0, double f1, int order, int n_arg, double *arg);

//
//	Filter table
//
static void 
error(const char *fmt, ...) {
   char buf[1024];
   va_list ap;
   va_start(ap, fmt);
   vsnprintf(buf, sizeof(buf), fmt, ap);	// Ignore overflow
   buf[sizeof(buf)-1]= 0;
   // If error handler routine returns, we dump to STDERR and exit anyway
   fprintf(stderr, "fidlib error: %s\n", buf);
   exit(1);
}
    void (*error_handler)(char *err) = nullptr;
public:
    void 
    set_error_handler(void (*rout)(char*));
    static const char * VERSION;
    const char *
    version();

struct Entry {
   FidFilter *(Fid::*rout)(double,double,double,int,int,double*); // Designer routine address
   const char *fmt;	// Format for spec-string
   const char *txt;	// Human-readable description of filter
};
static Entry filter[];
//
//	Design a filter.  Spec and range are passed as arguments.  The
//	return value is a pointer to a FidFilter as documented earlier
//	in this file.  This needs to be free()d once finished with.
//
//	If 'f_adj' is set, then the frequencies fed to the design code
//	are adjusted automatically to get true sqrt(0.5) (-3.01dB)
//	values at the provided frequencies.  (This is obviously a
//	slower operation)
//
//	If 'descp' is non-0, then a long description of the filter is
//	generated and returned as a strdup'd string at the given
//	location.
//
//	Any problem with the spec causes the program to die with an
//	error message.
//
//	'spec' gives the specification string.  The 'rate' argument
//	gives the sampling rate for the data that will be passed to
//	the filter.  This is only used to interpret the frequencies
//	given in the spec or given in 'freq0' and 'freq1'.  Use 1.0 if
//	the frequencies are given as a proportion of the sampling
//	rate, in the range 0 to 0.5.  'freq0' and 'freq1' provide the
//	default frequency or frequency range if this is not included
//	in the specification string.  These should be -ve if there is
//	no default range (causing an error if they are omitted from
//	the 'spec').
//
struct Spec {
#define MAXARG 10
   const char *spec;
   double in_f0, in_f1;
   int in_adj;
   double argarr[MAXARG];
   double f0, f1;
   int adj;
   int n_arg;
   int order;
   int minlen;		// Minimum length of spec-string, assuming f0/f1 passed separately
   int n_freq;		// Number of frequencies provided: 0,1,2
   int fi;		// Filter index (filter[fi])
};


FidFilter *
design(const char *spec, double rate, double freq0, double freq1, int f_adj, char **descp);
//	Auto-adjust input frequency to give correct sqrt(0.5)
//	(~-3.01dB) point to 6 figures
//

#define M301DB (0.707106781186548)
FidFilter * auto_adjust_single(Spec *sp, double rate, double f0); 
//
//	Auto-adjust input frequencies to give response of sqrt(0.5)
//	(~-3.01dB) correct to 6sf at the given frequency-points
//

FidFilter * auto_adjust_dual(Spec *sp, double rate, double f0, double f1);
//
//	Expand a specification string to the given buffer; if out of
//	space, drops dead
//

void 
expand_spec(char *buf, char *bufend, const char *str);
//
//	Design a filter and reduce it to a list of all the non-const
//	coefficients.  Arguments are as for fid_filter().  The
//	coefficients are written into the given double array.  If the
//	number of coefficients doesn't match the array length given,
//	then a fatal error is generated.
//
//	Note that all 1-element FIRs and IIR first-coefficients are
//	merged into a single gain coefficient, which is returned
//	rather than being included in the coefficient list.  This is
//	to allow it to be merged with other gains within a stack of
//	filters.
//
//	The algorithm used here (merging 1-element FIRs and adjusting
//	IIR first-coefficients) must match that used in the code-
//	generating code, or else the coefficients won't match up.  The
//	'n_coef' argument provides a partial safeguard.
//

double 
design_coef(double *coef, int n_coef, const char *spec, double rate, 
		double freq0, double freq1, int adj);
//
//	List all the known filters to the given file handle
//

void 
list_filters(FILE *out);

//
//	List all the known filters to the given buffer; the buffer is
//	NUL-terminated; returns 1 okay, 0 not enough space
//

int 
list_filters_buf(char *buf, char *bufend);

//
//      Do a convolution of parameters in place
//

int
convolve(double *dst, int n_dst, double *src, int n_src);

//
//	Generate a combined filter -- merge all the IIR/FIR
//	sub-filters into a single IIR/FIR pair, and make sure the IIR
//	first coefficient is 1.0.
//

FidFilter *
flatten(FidFilter *filt);
//	Parse a filter-spec and freq0/freq1 arguments.  Returns a
//	strdup'd error string on error, or else 0.
//

char *
parse_spec(Spec *sp);
//
//	Parse a filter-spec and freq0/freq1 arguments and rewrite them
//	to give an all-in-one filter spec and/or a minimum spec plus
//	separate freq0/freq1 arguments.  The all-in-one spec is
//	returned in *spec1p (strdup'd), and the minimum separated-out
//	spec is returned in *spec2p (strdup'd), *freq0p and *freq1p.
//	If either of spec1p or spec2p is 0, then that particular
//	spec-string is not generated.
//

void 
rewrite_spec(const char *spec, double freq0, double freq1, int adj,
		 char **spec1p, 
		 char **spec2p, double *freq0p, double *freq1p, int *adjp);
//	Create a FidFilter from the given double array.  The double[]
//	should contain one or more sections, each starting with the
//	filter type (either 'I' or 'F', as a double), then a count of
//	the number of coefficients following, then the coefficients
//	themselves.  The end of the list is marked with a type of 0.
//
//	This is really just a convenience function, allowing a filter
//	to be conveniently dumped to C source code and then
//	reconstructed.  
//
//	Note that for more general filter generation, FidFilter
//	instances can be created simply by allocating the memory and
//	filling them in (see fidlib.h).  
//

FidFilter *
cv_array(double *arr);

//
//	Create a single filter from the given list of filters in
//	order.  If 'freeme' is set, then all the listed filters are
//	free'd once read; otherwise they are left untouched.  The
//	newly allocated resultant filter is returned, which should be
//	released with free() when finished with.
//      

FidFilter *
cat(int freeme, ...);
//	Support for fid_parse
//

// Skip white space (including comments)
void 
skipWS(char **pp);

// Grab a word from the input into the given buffer.  Returns 0: end
// of file or error, else 1: success.  Error is indicated when the
// word doesn't fit in the buffer.
int 
grabWord(char **pp, char *buf, int buflen);

//
//	Parse an entire filter specification, perhaps consisting of
//	several FIR, IIR and predefined filters.  Stops at the first
//	,; or unmatched )]}.  Returns either 0 on success, or else a
//	strdup'd error string.
//
//	This duplicates code from Fiview filter.c, I know, but this
//	may have to expand in the future to handle '+' operations, and
//	special filter types like tunable heterodyne filters.  At that
//	point, the filter.c code will have to be modified to call a
//	version of this routine.
//

    char *
    parse(double rate, char **pp, FidFilter **ffp);
    void *
    run_new(FidFilter *filt, double (**funcpp)(void *,double));
    void *
    run_newbuf(void *run);
    void
    run_zapbuf(void *buf);
    void 
    run_freebuf(void *runbuf);
    void 
    run_free(void *run);

};

