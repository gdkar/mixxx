#include "util/rdtsc.hpp"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <atomic>
#include <x86intrin.h>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <thread>
#include <mutex>
namespace {
inline uint64_t rdtscp(void)
{
    auto ignored = uint32_t{};
    return __rdtscp(&ignored);
}
inline uint64_t rdtsc(void)
{
    return __rdtsc();
}
std::atomic<float> factor{ 1.0f};
std::atomic<uint64_t> offset{};
std::once_flag factor_callibrate_once{};

float estimate_tick_factor(int usec)
{
    auto ts_start = timespec{}, ts_stop = timespec{};

    auto start = rdtsc();
    clock_gettime(CLOCK_REALTIME, &ts_start);
    ::usleep(usec);
    auto count = rdtscp() - start;
    clock_gettime(CLOCK_REALTIME, &ts_stop);
    auto actual_ns = (ts_stop.tv_sec - ts_start.tv_sec) * 1e9 + (ts_stop.tv_nsec - ts_start.tv_nsec);
    return actual_ns / count;
}
float update_tick_factor(int usec)
{
    auto estimate = estimate_tick_factor(usec);
    auto expected = factor.load();
    while(true) {
        auto desired  = (expected ? (expected + estimate)*0.5f : estimate);
        if(factor.compare_exchange_strong(expected, desired))
            break;
    }
    return estimate;
}
void init_tick_factor(void)
{
    auto init_factor = update_tick_factor(1<<16);
    auto samples = std::array<double, 16>{};
    auto sum = 0.;
    auto c   = 0.;
    auto avg = 0.;
    for(auto & sample:samples){
        sample = update_tick_factor(4096);
        auto y = sample - c;
        auto t = sum + y;
             c = (t-sum) - y;
           sum = t;
    }
    avg = sum / samples.size();
    sum = 0, c = 0;
    for(auto sample : samples) {
        auto y = (sample - avg) * (sample - avg) - c;
        auto t = sum + y;
             c = (t - sum) - y;
           sum = t;
    }
    auto variance = sum / (samples.size() - 1u);
    std::cerr << "factor: "  << init_factor << " mean: " << avg << " standard deviation: " << std::sqrt(variance) << " relative std: " << (std::sqrt(variance)/avg) << std::endl;
    if(std::max(
        std::abs(avg - init_factor) /(0.5 * ( avg + init_factor)),
        std::sqrt(variance) / avg
        )> 1e-3) {
        std::cerr << "timing variance greater than .1% or mean drifted by > .1%\n";
    }
}
struct once_callibration {
    once_callibration(int)
    {
        std::call_once(factor_callibrate_once, init_tick_factor);
        offset = rdtscp();
    }
} calibration{0};
}
namespace mixxx {
uint64_t get_ticks(void)
{
    return rdtscp();
}
uint64_t get_ticks_relative(void)
{
    return rdtscp() - offset.load(std::memory_order_relaxed);
}
uint64_t get_ticks_ns(void)
{
    return get_ticks() * factor.load(std::memory_order_relaxed);
}
uint64_t get_ticks_ns_relative(void)
{
    return get_ticks_relative() * factor.load(std::memory_order_relaxed);
}
}
