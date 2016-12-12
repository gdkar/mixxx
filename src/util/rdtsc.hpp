#ifndef UTIL_RDTSC_HPP
#define UTIL_RDTSC_HPP

#include <climits>
#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <ctime>
#include <x86intrin.h>

namespace mixxx {
int usleep_update(useconds_t usec);
uint64_t get_ticks(void);
uint64_t get_ticks_relative(void);
uint64_t get_ticks_ns(void);
uint64_t get_ticks_ns_relative(void);
uint64_t get_clock_ns(void);
uint64_t get_clock_ns_relative(void);
inline void cpu_relax(void)
{
    _mm_pause();
}
template<int N>
inline void cpu_relax(void)
{
    for(auto i = N; i--; _mm_pause());
}
}
#endif
