// Taken from https://github.com/bstreiff/cppbits
// Thank you Brandon Streiff!

// Implementation of C++14's make_unique for C++11 compilers.
//
// This has been tested with:
// - MSVC 11.0 (Visual Studio 2012)
// - gcc 4.6.3
// - Xcode 4.4 (with clang "4.0")
//
// It is based off an implementation proposed by Stephan T. Lavavej for
// inclusion in the C++14 standard:
//    http://isocpp.org/files/papers/N3656.txt
// Where appropriate, it borrows the use of MSVC's _VARIADIC_EXPAND_0X macro
// machinery to compensate for lack of variadic templates.
//
// This file injects make_unique into the std namespace, which I acknowledge is
// technically forbidden ([C++11: 17.6.4.2.2.1/1]), but is necessary in order
// to have syntax compatibility with C++14.
//
// I perform compiler version checking for MSVC, gcc, and clang to ensure that
// we don't add make_unique if it is already there (instead, we include
// <memory> to get the compiler-provided one). You can override the compiler
// version checking by defining the symbol COMPILER_SUPPORTS_MAKE_UNIQUE.
//
//
// ===============================================================================
// This file is released into the public domain. See LICENCE for more information.
// ===============================================================================

#ifndef MIXXX_UTIL_MEMORY_H
#define MIXXX_UTIL_MEMORY_H

// If the compiler supports std::make_unique, then pull in <memory> to get it.
#include <memory>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <iterator>
#include <functional>
#include <algorithm>

#endif /* MIXXX_UTIL_MEMORY_H */
