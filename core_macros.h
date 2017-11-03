#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#if defined(__has_include)
	#if !__has_include(<boost/config.hpp>)
	#define OEL_NO_BOOST 1
	#endif
#endif


#if defined(_MSC_VER) && _MSC_VER < 1900 && !defined(__llvm__)
	#ifndef _ALLOW_KEYWORD_MACROS
	#define _ALLOW_KEYWORD_MACROS 1
	#endif

	#ifndef noexcept
	#define noexcept throw()
	#endif

	#undef constexpr
	#define constexpr

	#define OEL_ALIGNOF __alignof
#else
	#define OEL_ALIGNOF alignof
#endif

#if defined(NDEBUG) && OEL_MEM_BOUND_DEBUG_LVL == 0
	#define OEL_NOEXCEPT_NDEBUG noexcept
#else
	#define OEL_NOEXCEPT_NDEBUG
#endif


#if _MSC_VER
	#define OEL_CONST_COND __pragma(warning(suppress : 4127))
#else
	#define OEL_CONST_COND
#endif

////////////////////////////////////////////////////////////////////////////////

#if !defined(NDEBUG) && !defined(OEL_MEM_BOUND_DEBUG_LVL)
/** @brief Undefined/0: no array index and iterator checks. 1: most debug checks. 2: all checks, often slow.
*
* 0 (or undefined by NDEBUG defined) is not binary compatible with levels 1 and 2. */
	#define OEL_MEM_BOUND_DEBUG_LVL 2
#endif


#ifndef OEL_ALWAYS_ASSERT
	#if defined(__has_builtin)
	#define OEL_HAS_BUILTIN_TRAP __has_builtin(__builtin_trap)
	#else
	#define OEL_HAS_BUILTIN_TRAP defined(__GNUC__)
	#endif

	#ifndef OEL_HALT
	/** Could throw an exception instead, or do whatever. If it contains assembly, compilation will likely fail.
	*
	* Example: @code
	#define OEL_HALT(failedCond) throw std::logic_error(failedCond ", assertion failed in " __FILE__)
	@endcode  */
		#if _MSC_VER
		#define OEL_HALT(failedCond) __debugbreak()
		#elif OEL_HAS_BUILTIN_TRAP
		#define OEL_HALT(failedCond) __builtin_trap()
		#else
		#define OEL_HALT(failedCond) std::abort()
		#endif
	#endif

	#undef OEL_HAS_BUILTIN_TRAP

	//! Executes OEL_HALT on failure. Can be defined to standard assert or your own
	#define OEL_ALWAYS_ASSERT(expr)  \
		OEL_CONST_COND  \
		do {  \
			if (!(expr)) OEL_HALT(#expr);  \
		} while (false)
#endif


#if OEL_MEM_BOUND_DEBUG_LVL
	#define OEL_ASSERT_MEM_BOUND  OEL_ALWAYS_ASSERT
#else
	#define OEL_ASSERT_MEM_BOUND(expr) ((void) 0)
#endif
#if !defined(NDEBUG)
	#define OEL_ASSERT  OEL_ALWAYS_ASSERT
#else
	#define OEL_ASSERT(expr) ((void) 0)
#endif

////////////////////////////////////////////////////////////////////////////////

#if defined(_CPPUNWIND) || defined(__EXCEPTIONS)
	#define OEL_THROW(exception)      throw exception
	#define OEL_TRY_                  try
	#define OEL_CATCH_ALL             catch (...)
	#define OEL_WHEN_EXCEPTIONS_ON(x) x
#else
	#define OEL_THROW(exception)      std::terminate()
	#define OEL_TRY_
	#define OEL_CATCH_ALL             OEL_CONST_COND if (false)
	#define OEL_WHEN_EXCEPTIONS_ON(x)
#endif
