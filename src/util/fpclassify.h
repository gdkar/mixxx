#ifndef UTIL_FPCLASSIFY_H
#define UTIL_FPCLASSIFY_H

// We define 
// these as macros to prevent clashing with c++11 built-ins in the global
// namespace. If you say "using std::isnan;" then this will fail to build with
// std=c++11. See https://bugs.webkit.org/show_bug.cgi?id=59249 for some
// relevant discussion.
#include <cfloat>
#include <cmath>
#include <limits>


using std::isfinite;
using std::isnormal;
using std::isnan;
using std::fpclassify;
using std::isinf;

#endif // UTIL_FPCLASSIFY_H
