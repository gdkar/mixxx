
#ifndef NAN_INF_H
#define NAN_INF_H

#define ISNAN(x) (sizeof(x) == sizeof(double) ? ISNANd(x) : ISNANf(x))
static constexpr bool ISNANf(float x) { return x != x; }
static constexpr bool ISNANd(double x) { return x != x; }

#define ISINF(x) (sizeof(x) == sizeof(double) ? ISINFd(x) : ISINFf(x))
static constexpr bool ISINFf(float x) { return !ISNANf(x) && ISNANf(x - x); }
static constexpr bool ISINFd(double x) { return !ISNANd(x) && ISNANd(x - x); }

#endif
