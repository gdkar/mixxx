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
#include <sys/time.h>
#include <unistd.h>
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
std::atomic<double>    ticks_factor{ 1.0};
std::atomic<uint64_t> ticks_offset{};
std::atomic<timespec> clock_offset{};
std::once_flag factor_callibrate_once{};

std::tuple<double,uint64_t,timespec> estimate_tick_factor(useconds_t usec)
{
    auto ts_start = timespec{}, ts_stop = timespec{};
    auto start = rdtsc();
    clock_gettime(CLOCK_REALTIME, &ts_start);
    ::usleep(usec);
    auto count = rdtscp() - start;
    clock_gettime(CLOCK_REALTIME, &ts_stop);
    auto actual_ns = (ts_stop.tv_sec - ts_start.tv_sec) * 1e9 + (ts_stop.tv_nsec - ts_start.tv_nsec);
    return std::make_tuple(actual_ns / count,start,ts_start);
}
std::tuple<double,uint64_t,timespec> long_term_tick_factor(useconds_t usec)
{
    auto start = ticks_offset.load(std::memory_order_relaxed);
    auto ts_start = clock_offset.load(std::memory_order_relaxed);
    auto ts_stop = timespec{};
    if(!start || !(ts_start.tv_sec || ts_start.tv_nsec))
        return estimate_tick_factor(usec);
    ::usleep(usec);
    auto stop = rdtscp();
    auto count = stop - start;
    clock_gettime(CLOCK_REALTIME, &ts_stop);
    auto actual_ns = (ts_stop.tv_sec - ts_start.tv_sec) * 1e9 + (ts_stop.tv_nsec - ts_start.tv_nsec);
    return std::make_tuple(actual_ns / count,start,ts_start);
}
double update_tick_factor(useconds_t usec, bool long_term = false)
{
    auto values = long_term ? long_term_tick_factor(usec) : estimate_tick_factor(usec);
    auto estimate = std::get<0>(values);
    {
        auto start_expected = ticks_offset.load(std::memory_order_acquire);
        if(!start_expected){
            if(ticks_offset.compare_exchange_strong(start_expected, std::get<1>(values)))
                clock_offset.store(std::get<2>(values));
        }
    }
    {
        auto expected = ticks_factor.load();
        while(true) {
            auto desired  = (expected ? (expected * 0.9+ estimate * 0.1) : estimate);
            if(ticks_factor.compare_exchange_strong(expected, desired))
                break;
        }
    }
    return estimate;
}
void init_tick_factor(void)
{
    auto init_factor = update_tick_factor(1<<16);
    auto samples = std::array<double, 8>{};
    auto sum = 0.;
    auto c   = 0.;
    auto avg = 0.;
    for(auto & sample:samples){
        sample = update_tick_factor(4096,false);
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
        )> 1e-6) {
        std::cerr << "\n\ntiming variance greater than 1e-6 or mean drifted by > 1e-6\n\n";
    }
}
struct once_callibration {
    once_callibration(int)
    {
        std::call_once(factor_callibrate_once, init_tick_factor);
    }
} calibration{0};
}
namespace mixxx {
int usleep_update(useconds_t usecs)
{
    update_tick_factor(usecs, usecs > 16000);
    return 0;
}
uint64_t get_ticks(void)
{
    return rdtscp();
}
uint64_t get_ticks_relative(void)
{
    return rdtscp() - ticks_offset.load(std::memory_order_relaxed);
}
uint64_t get_clock_ns(void)
{
    auto ts = timespec{};
    clock_gettime(CLOCK_REALTIME,&ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

uint64_t get_ticks_ns(void)
{
    return get_ticks() * ticks_factor.load(std::memory_order_relaxed);
}
uint64_t get_ticks_ns_relative(void)
{
    return get_ticks_relative() * ticks_factor.load(std::memory_order_relaxed);
}
uint64_t get_clock_ns_relative(void)
{
    auto ts_end = timespec{};
    clock_gettime(CLOCK_REALTIME,&ts_end);
    auto ts_beg = clock_offset.load(std::memory_order_relaxed);
    return (ts_end.tv_sec -ts_beg.tv_sec) * 1e9 + (ts_end.tv_nsec - ts_beg.tv_nsec);
}
}
