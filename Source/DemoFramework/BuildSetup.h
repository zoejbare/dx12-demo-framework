/**
 * Copyright (c) 2023, Zoe J. Bare
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

/*********************************************************************************************************************/

#define DF_VERSION_MAJOR 1
#define DF_VERSION_MINOR 0
#define DF_VERSION_PATCH 0

#define _DF_INTERNAL_VERSION_STR2(x) #x
#define _DF_INTERNAL_VERSION_STR(x) _DF_INTERNAL_VERSION_STR2(x)

#define DF_VERSION_STRING \
	_DF_INTERNAL_VERSION_STR(DF_VERSION_MAJOR) "." \
	_DF_INTERNAL_VERSION_STR(DF_VERSION_MINOR) "." \
	_DF_INTERNAL_VERSION_STR(DF_VERSION_PATCH)

/*********************************************************************************************************************/

#include <assert.h>
#include <float.h>
#include <inttypes.h>
#include <stdlib.h>

/*********************************************************************************************************************/

typedef float  float32_t;
typedef double float64_t;

/*********************************************************************************************************************/

#define DF_CPU_32_BIT 0
#define DF_CPU_64_BIT 0

/*********************************************************************************************************************/

/* Needed for the _WIN32_WINNT macros. */
#include <sdkddkver.h>

/*-------------------------------------------------------------------------------------------------------------------*/

#if defined(_WIN32_WINNT)
	/* Versions earlier than Windows 10 are not supported. */
	#undef  _WIN32_WINNT
	#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

/* Strip out the unnecessary stuff from the main Windows header. */
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

/*********************************************************************************************************************/

#include <Windows.h>
#include <objbase.h>

/*********************************************************************************************************************/

#if defined(_M_X64) || defined(__x86_64__)
	#undef  GB_CPU_64_BIT
	#define GB_CPU_64_BIT 1

#else
	#undef  GB_CPU_32_BIT
	#define GB_CPU_32_BIT 1

#endif

/*********************************************************************************************************************/

#define DF_ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

#define DF_UNUSED(x) ((void)(x))

/*********************************************************************************************************************/

#if defined(DF_DLL_EXPORT)
	#define DF_API __declspec(dllexport)

#else
	#define DF_API __declspec(dllimport)

#endif

/*********************************************************************************************************************/

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

#ifdef Yield
	#undef Yield
#endif

#ifdef CreateEvent
	#undef CreateEvent
#endif

/*********************************************************************************************************************/
