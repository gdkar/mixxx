
#ifndef NAN_INF_H
#define NAN_INF_H

#define ISNAN(x) (sizeof(x) == sizeof(float) ? ISNANd(x) : ISNANf(x))
static inline int ISNANf(float x) { return x != x; }
static inline int ISNANd(float x) { return x != x; }
          
#define ISINF(x) (sizeof(x) == sizeof(float) ? ISINFd(x) : ISINFf(x))
static inline int ISINFf(float x) { return !ISNANf(x) && ISNANf(x - x); }
static inline int ISINFd(float x) { return !ISNANd(x) && ISNANd(x - x); }

#endif
