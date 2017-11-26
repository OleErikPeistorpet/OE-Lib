#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#if defined(_MSC_VER) && _MSC_VER < 1900 && !defined(__llvm__)
	#ifndef _ALLOW_KEYWORD_MACROS
	#define _ALLOW_KEYWORD_MACROS 1
	#endif

	#undef constexpr
	#define constexpr

	#ifndef noexcept
	#define noexcept throw()
	#endif

	#define OEL_NOEXCEPT(expr)

	#define OEL_ALIGNOF __alignof
#else
	#define OEL_NOEXCEPT(expr) noexcept(expr)

	#define OEL_ALIGNOF alignof
#endif


#ifdef __GNUC__
	#define OEL_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#else
	#define OEL_GCC_VERSION 0
#endif

#ifdef __has_builtin
	#define OEL_HAS_BUILTIN_TRAP __has_builtin(__builtin_trap)
#elif __GNUC__
	#define OEL_HAS_BUILTIN_TRAP 1
#else
	#define OEL_HAS_BUILTIN_TRAP 0
#endif


#ifdef _MSC_VER
	#define OEL_CONST_COND __pragma(warning(suppress : 4127))

	#define OEL_NORETURN __declspec(noreturn)
#else
	#define OEL_CONST_COND

	#define OEL_NORETURN __attribute__((noreturn))
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
