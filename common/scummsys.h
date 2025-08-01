/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef COMMON_SCUMMSYS_H
#define COMMON_SCUMMSYS_H

#ifndef __has_feature           // Optional of course.
	#define __has_feature(x) 0  // Compatibility with non-clang compilers.
#endif

// This is a convenience macro to test whether the compiler used is a GCC
// version, which is at least major.minor.  Note that Clang will also define
// it and report itself as GCC 4.2.1.
#ifdef __GNUC__
	#define GCC_ATLEAST(major, minor) (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
	#define GCC_ATLEAST(major, minor) 0
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
	#error MSVC support requires MSVC 2015 or newer
#endif

#if defined(NONSTANDARD_PORT)

	// Ports which need to perform #includes and #defines visible in
	// virtually all the source of ScummVM should do so by providing a
	// "portdefs.h" header file (and not by directly modifying this
	// header file).
	#include <portdefs.h>
#else // defined(NONSTANDARD_PORT)

	#if defined(WIN32)

		#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
		#define NOGDICAPMASKS
		#define OEMRESOURCE
		#define NONLS
		#define NOICONS
		#define NOMCX
		#define NOPROFILER
		#define NOKANJI
		#define NOSERVICE
		#define NOMETAFILE
		#define NOCOMM
		#define NOCRYPT
		#define NOIME
		#define NOATOM
		#define NOCTLMGR
		#define NOCLIPBOARD
		#define NOMEMMGR
		#define NOSYSMETRICS
		#define NOMENUS
		#define NOOPENFILE
		#define NOWH
		#define NOSOUND
		#define NODRAWTEXT
		#define NOMINMAX 1

	#endif

	#if defined(__QNXNTO__)
	#include <strings.h>	/* For strcasecmp */
	#endif

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <stdarg.h>
	#include <stddef.h>
	#include <assert.h>
	#include <ctype.h>

	// The C++11 standard removed the C99 requirement that some <inttypes.h>
	// features should only be available when the following macros are defined.
	// But on some systems (such as RISC OS or macOS < 10.7), the system headers
	// are not necessarily up to date with this change. So, continue defining
	// them, in order to avoid build failures on some environments.
	#define __STDC_CONSTANT_MACROS
	#define __STDC_FORMAT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <inttypes.h>

	// macOS 10.4 inttypes.h woes -- fixed in 10.5 SDK
	#if defined(MACOSX) && defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1050
		#undef __PRI_8_LENGTH_MODIFIER__
		#undef __PRI_64_LENGTH_MODIFIER__
		#undef __SCN_64_LENGTH_MODIFIER__
		#define __PRI_8_LENGTH_MODIFIER__ "hh"
		#define __PRI_64_LENGTH_MODIFIER__ "ll"
		#define __SCN_64_LENGTH_MODIFIER__ "ll"
	#endif

	#include <limits.h>
	// MSVC does not define M_PI, M_SQRT2 and other math defines by default.
	// _USE_MATH_DEFINES must be defined in order to have these defined, thus
	// we enable it here. For more information, check:
	// https://docs.microsoft.com/en-us/cpp/c-runtime-library/math-constants
	#define _USE_MATH_DEFINES
	#include <math.h>

	// FIXME: We sadly can't assume standard C++ headers to be present on every
	// system we support, so we should get rid of this. The solution should be to
	// write a simple placement new on our own. It might be noteworthy we can't
	// easily do that for systems which do have a <new>, since it might clash with
	// the default definition otherwise!
	#include <new>

	// After having discussion about use of std::numeric_limits in
	// https://github.com/scummvm/scummvm/pull/3966 we concluded that it is safe to
	// use it as minimal STL code with low probability to bring any incompatibilities
	#include <limits>
#endif

#ifndef STATIC_ASSERT
#if __cplusplus >= 201103L || defined(_MSC_VER)
	/**
	 * Generates a compile-time assertion.
	 *
	 * @param expression An expression that can be evaluated at compile time.
	 * @param message An underscore-delimited message to be presented at compile
	 * time if the expression evaluates to false.
	 */
	#define STATIC_ASSERT(expression, message) \
		static_assert((expression), #message)
#else
	/**
	 * Generates a compile-time assertion.
	 *
	 * @param expression An expression that can be evaluated at compile time.
	 * @param message An underscore-delimited message to be presented at compile
	 * time if the expression evaluates to false.
	 */
	#define STATIC_ASSERT(expression, message) \
		do { \
			extern int STATIC_ASSERT_##message[(expression) ? 1 : -1]; \
			(void)(STATIC_ASSERT_##message); \
		} while (false)
#endif
#endif

// The following math constants are usually defined by the system math.h header, but
// they are not part of the ANSI C++ standards and so can NOT be relied upon to be
// present i.e. when -std=c++11 is passed to GCC, enabling strict ANSI compliance.
// As we rely on these being present, we define them if they are not set.

#ifndef M_E
	#define M_E 2.7182818284590452354 /* e */
#endif

#ifndef M_LOG2E
	#define M_LOG2E 1.4426950408889634074 /* log_2 e */
#endif

#ifndef M_LOG10E
	#define M_LOG10E 0.43429448190325182765 /* log_10 e */
#endif

#ifndef M_LN2
	#define M_LN2 0.69314718055994530942 /* log_e 2 */
#endif

#ifndef M_LN10
	#define M_LN10 2.30258509299404568402 /* log_e 10 */
#endif

#ifndef M_PI
	#define M_PI 3.14159265358979323846 /* pi */
#endif

#ifndef M_PI_2
	#define M_PI_2 1.57079632679489661923 /* pi/2 */
#endif

#ifndef M_PI_4
	#define M_PI_4 0.78539816339744830962 /* pi/4 */
#endif

#ifndef M_1_PI
	#define M_1_PI 0.31830988618379067154 /* 1/pi */
#endif

#ifndef M_2_PI
	#define M_2_PI 0.63661977236758134308 /* 2/pi */
#endif

#ifndef M_2_SQRTPI
	#define M_2_SQRTPI 1.12837916709551257390 /* 2/sqrt(pi) */
#endif

#ifndef M_SQRT2
	#define M_SQRT2 1.41421356237309504880 /* sqrt(2) */
#endif

#ifndef M_SQRT1_2
	#define M_SQRT1_2 0.70710678118654752440 /* 1/sqrt(2) */
#endif

// Use config.h, generated by configure
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif


// In the following we configure various targets, in particular those
// which can't use our "configure" tool and hence don't use config.h.
//
// Some #defines that occur here frequently:
// SCUMM_LITTLE_ENDIAN
//    - Define this on a little endian target
// SCUMM_BIG_ENDIAN
//    - Define this on a big endian target
// SCUMM_NEED_ALIGNMENT
//    - Define this if your system has problems reading e.g. an int32 from an odd address
// SMALL_SCREEN_DEVICE
//    - ...
// ...

//
// If -fsanitize=alignment is in use (or UBSan enabled it), and the compiler
// happens to report it, make sure SCUMM_NEED_ALIGNMENT is defined, in order
// to avoid false positives. `alignment_sanitizer` may not be reported in
// earlier releases though, so look for the UBSan flag in that case.
//
#if __has_feature(alignment_sanitizer) || __has_feature(undefined_behavior_sanitizer)
	#define SCUMM_NEED_ALIGNMENT
#endif

//
// By default we try to use pragma push/pop to ensure various structs we use
// are "packed". If your compiler doesn't support this pragma, you are in for
// a problem. If you are lucky, there is a compiler switch, or another pragma,
// doing the same thing -- in that case, try to modify common/pack-begin.h and
// common/pack-end.h accordingly. Or maybe your port simply *always* packs
// everything, in which case you could #undefine SCUMMVM_USE_PRAGMA_PACK.
//
// If neither is possible, tough luck. Try to contact the team, maybe we can
// come up with a solution, though I wouldn't hold my breath on it :-/.
//
#define SCUMMVM_USE_PRAGMA_PACK

//
// Determine the host endianess and whether memory alignment is required.
//
#if !defined(HAVE_CONFIG_H)

	#if defined(__DC__) || \
		  defined(__DS__) || \
		  defined(__3DS__) || \
		  defined(IPHONE) || \
		  defined(__PSP__)

		#define SCUMM_LITTLE_ENDIAN
		#define SCUMM_NEED_ALIGNMENT

	#elif defined(_MSC_VER) || defined(__MINGW32__)

		#define SCUMM_LITTLE_ENDIAN

	#elif defined(__MORPHOS__) || defined(__amigaos4__) || defined(__N64__) || defined(__WII__)

		#define SCUMM_BIG_ENDIAN
		#define SCUMM_NEED_ALIGNMENT

	#elif defined(SDL_BACKEND)
		// On SDL based ports, we try to use SDL_BYTEORDER to determine the
		// endianess. We explicitly do this as the *last* thing we try, so that
		// platform specific settings have precedence.
		#include <SDL_endian.h>

		#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		#define SCUMM_LITTLE_ENDIAN
		#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
		#define SCUMM_BIG_ENDIAN
		#else
		#error Neither SDL_BIG_ENDIAN nor SDL_LIL_ENDIAN is set.
		#endif

	#else

		#error No system type defined, host endianess unknown.

	#endif
#endif

#if !defined(SCUMM_FLOAT_WORD_LITTLE_ENDIAN) && !defined(SCUMM_FLOAT_WORD_BIG_ENDIAN)

	#if defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__) && defined(__FLOAT_WORD_ORDER__)

		#if (__FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__)
			#define SCUMM_FLOAT_WORD_LITTLE_ENDIAN
		#elif (__FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__)
			#define SCUMM_FLOAT_WORD_BIG_ENDIAN
		#else
			#error Unsupported endianness
		#endif

	#else
		#ifdef SCUMM_LITTLE_ENDIAN
			#define SCUMM_FLOAT_WORD_LITTLE_ENDIAN
		#else
			#define SCUMM_FLOAT_WORD_BIG_ENDIAN
		#endif

	#endif
#endif

#if defined(USE_TREMOR) && !defined(USE_VORBIS)
#define USE_VORBIS // make sure this one is defined together with USE_TREMOR!
#endif

//
// Fallbacks / default values for various special macros
//
#ifndef GCC_PRINTF
	#if defined(__GNUC__) || defined(__INTEL_COMPILER)
		#if defined(__USE_MINGW_ANSI_STDIO) && __USE_MINGW_ANSI_STDIO && !defined(__clang__)
			#define GCC_PRINTF(x,y) __attribute__((__format__(__gnu_printf__, x, y)))
		#else
			#define GCC_PRINTF(x,y) __attribute__((__format__(__printf__, x, y)))
		#endif
	#else
		#define GCC_PRINTF(x,y)
	#endif
#endif

#ifndef MSVC_PRINTF
	#if defined(_MSC_VER)
		#define MSVC_PRINTF _Printf_format_string_
	#else
		#define MSVC_PRINTF
	#endif
#endif

#ifndef PACKED_STRUCT
	#if defined(__GNUC__) || defined(__INTEL_COMPILER)
		#define PACKED_STRUCT __attribute__((__packed__))
	#else
		#define PACKED_STRUCT
	#endif
#endif

#ifndef FORCEINLINE
	#if defined(_MSC_VER)
		#define FORCEINLINE __forceinline
	#elif defined(__GNUC__)
		#define FORCEINLINE inline __attribute__((__always_inline__))
	#else
		#define FORCEINLINE inline
	#endif
#endif

#ifndef PLUGIN_EXPORT
	#if defined(_MSC_VER) || defined(__MINGW32__)
		#define PLUGIN_EXPORT __declspec(dllexport)
	#else
		#define PLUGIN_EXPORT
	#endif
#endif

#ifndef NORETURN_PRE
	#if defined(_MSC_VER)
		#define NORETURN_PRE __declspec(noreturn)
	#elif defined(__GNUC__)
		#define NORETURN_PRE __attribute__((__noreturn__))
	#else
		#define NORETURN_PRE
	#endif
#endif

#ifndef NORETURN_POST
	#if defined(__INTEL_COMPILER)
		#define NORETURN_POST __attribute__((__noreturn__))
	#else
		#define NORETURN_POST
	#endif
#endif

#ifndef WARN_UNUSED_RESULT
	#if __cplusplus >= 201703L
		#define WARN_UNUSED_RESULT [[nodiscard]]
	#elif defined(__GNUC__)
		#define WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
	#elif defined(_Check_return_)
		#define WARN_UNUSED_RESULT _Check_return_
	#else
		#define WARN_UNUSED_RESULT
	#endif
#endif

#ifndef WARN_DEPRECATED
	#if __cplusplus >= 201703L
		#define WARN_DEPRECATED(msg) [[deprecated(msg)]]
	#elif defined(__GNUC__)
		#define WARN_DEPRECATED(msg) __attribute__((__deprecated__(msg)))
	#elif defined(_MSC_VER)
		#define WARN_DEPRECATED(msg) __declspec(deprecated(msg))
	#else
		#define WARN_DEPRECATED(msg)
	#endif
#endif

#ifndef STRINGBUFLEN
	#if defined(__N64__) || defined(__DS__) || defined(__3DS__)
		#define STRINGBUFLEN 256
	#else
		#define STRINGBUFLEN 1024
	#endif
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif

#ifndef scumm_va_copy
	#if defined(va_copy)
		#define scumm_va_copy va_copy
	#elif defined(__va_copy)
		#define scumm_va_copy __va_copy
	#elif defined(_MSC_VER)
		#define scumm_va_copy(dst, src)       ((dst) = (src))
	#else
		#error scumm_va_copy undefined for this port
	#endif
#endif



//
// Typedef our system types unless SCUMMVM_DONT_DEFINE_TYPES is set.
// This is usually due to conflicts with the system headers.
//
#ifndef SCUMMVM_DONT_DEFINE_TYPES
	typedef unsigned char byte;
	typedef unsigned int uint;

	typedef uint8_t uint8;
	typedef int8_t int8;
	typedef uint16_t uint16;
	typedef int16_t int16;
#if defined(__3DS__) || defined(__DC__) || defined(__PSP__)
	/**
	 * The system headers define uint32_t and int32_t as long while int is enough
	 * Force use of int to avoid errors on format strings.
	 */
	typedef unsigned int uint32;
	typedef int int32;
#elif defined(__amigaos4__) || defined(__MORPHOS__)
	/**
	 * The system headers define uint32 and int32 as long, so we have to do the same here.
	 * Without this, we get a conflicting declaration error when this file is included
	 * along the AmigaOS files.
	 */
	typedef unsigned long uint32;
	typedef signed long int32;
#else
	typedef uint32_t uint32;
	typedef int32_t int32;
#endif
	typedef uint64_t uint64;
	typedef int64_t int64;
	typedef uintptr_t uintptr;
	typedef intptr_t intptr;
#endif

//
// std::nullptr_t when this type is not available
//
#if defined(NO_CXX11_NULLPTR_T)
namespace std {
	typedef decltype(nullptr) nullptr_t;
}
#endif

//
// std::initializer_list
// Provide replacement when not available
//
#if defined(NO_CXX11_INITIALIZER_LIST)
namespace std {
	template<class T> class initializer_list {
	public:
		typedef T value_type;
		typedef const T &reference;
		typedef const T &const_reference;
		typedef size_t size_type;
		typedef const T *iterator;
		typedef const T *const_iterator;

		constexpr initializer_list() noexcept = default;
		constexpr size_t size() const noexcept { return _size; };
		constexpr const T *begin() const noexcept { return _begin; };
		constexpr const T *end() const noexcept { return _begin + _size; }

	private:
		// Note: begin has to be first or the compiler may get very upset
		const T *_begin = { nullptr };
		size_t _size = { 0 };

		// The compiler is allowed to call this constructor
		constexpr initializer_list(const T* t, size_t s) noexcept : _begin(t) , _size(s) {}
	};

	template<class T> constexpr const T* begin(initializer_list<T> il) noexcept {
		return il.begin();
	}

	template<class T> constexpr const T* end(initializer_list<T> il) noexcept {
		return il.end();
	}
}

#else

#include <initializer_list>

#endif // NO_CXX11_INITIALIZER_LIST

//
// Some more system specific settings.
// TODO/FIXME: All of these should be moved to backend specific files (such as portdefs.h)
//
#if defined(DINGUX)

	// Very BAD hack following, used to avoid triggering an assert in uClibc dingux library
	// "toupper" when pressing keyboard function keys.
	#undef toupper
	#define toupper(c) __extension__ ({ auto _x = ((c) & 0xFF); (_x >= 97 && _x <= 122) ? (_x - 32) : _x; })

#elif defined(__PSP__)

	#include <malloc.h>
	#include "backends/platform/psp/memory.h"

	/* to make an efficient, inlined memcpy implementation */
	#define memcpy(dst, src, size)   psp_memcpy(dst, src, size)

#endif

#include "common/forbidden.h"

#endif
