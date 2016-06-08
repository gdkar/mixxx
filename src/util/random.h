#ifndef UTIL_RANDOM_H
#define UTIL_RANDOM_H

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <algorithm>
#include <utility>

struct XorShift128Plus {
    uint64_t s[2];
    constexpr XorShift128Plus() : s{0,0} {};
    constexpr XorShift128Plus(uint64_t _s0, uint64_t _s1)
        : s{ _s0, _s1} { }
    uint64_t operator() (void)
    {
        auto s1 = s[0];
        auto s0 = s[1];
        s[0]    = s0;
        s1 ^= s1 << 23;
        return (s[1] = (s1 ^ s0 ^ (s1 >> 16) ^ ( s0 >> 26))) + s0;
    }
};

struct XorShift1024Star {
    uint64_t s[16] = { 0u };
    uint32_t p{0};
    XorShift1024Star() = default;
    XorShift1024Star( uint64_t _s0, uint64_t _s1)
    {
        std::generate(std::begin(s),std::end(s),XorShift128Plus(_s0,_s1));
    }
    uint64_t operator()(void)
    {
        auto s0 = s[0];
        p = (p+1)&15;
        auto s1 = s[p];
        s1 ^= (s1<<31);
        s0 ^= (s0>>30);
        s1 ^= (s1>>11);
        return (s[p] = s0 ^ s1) * 1181783497276652981LL;
    }
};
#endif
