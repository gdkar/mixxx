/*  FastForward queue remix
 *  Copyright (c) 2011, Dmitry Vyukov
 *  Distributed under the terms of the GNU General Public License as published by the Free Software Foundation,
 *  either version 3 of the License, or (at your option) any later version.
 *  See: http://www.gnu.org/licenses
 */ 

#pragma once


#if (defined(_MSC_VER) || defined(__INTEL_COMPILER)) && defined(_WIN32)
#   include "platform_msvc_windows.h"
#elif defined(__GNUC__) && defined(__linux)
#   include "platform_gcc_linux.h"
#elif (defined(__SUNPRO_C) || defined(__SUNPRO_CC)) && defined(__sun)
#   include "platform_cc_solaris.h"
#else
#   error "unknown platform"
#endif

#include <memory.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

