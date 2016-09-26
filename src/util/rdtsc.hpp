#ifndef UTIL_RDTSC_HPP
#define UTIL_RDTSC_HPP

#include <climits>
#include <cstdio>
#include <cstdint>

namespace mixxx {
uint64_t get_ticks(void);
uint64_t get_ticks_relative(void);
uint64_t get_ticks_ns(void);
uint64_t get_ticks_ns_relative(void);
}
#endif
