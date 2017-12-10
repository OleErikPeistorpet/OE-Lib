#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/macros.h"


/** @file
*/

#if !defined(NDEBUG) && !defined(OEL_MEM_BOUND_DEBUG_LVL)
/** @brief Undefined/0: no array index and iterator checks. 1: most debug checks. 2: all checks, often slow.
*
* Level 1 gives failed assertions if you try to use dynarray iterators after swap or move construction.
* The different levels are not binary compatible, although mixing 0 and 1 typically works. */
	#define OEL_MEM_BOUND_DEBUG_LVL 2
#endif


//! Functions marked with OEL_NOEXCEPT_NDEBUG will only throw exceptions from OEL_ASSERT (none by default)
#if OEL_MEM_BOUND_DEBUG_LVL == 0
	#define OEL_NOEXCEPT_NDEBUG noexcept
#else
	#define OEL_NOEXCEPT_NDEBUG
#endif


#ifndef OEL_ABORT
	/** @brief Used by OEL_ASSERT, and anywhere that would normally throw if exceptions are disabled
	*
	* Users may define this to call a function that does not return or to throw an exception.
	* Example: @code
	#define OEL_ABORT(errorMessage)  throw std::logic_error(errorMessage "; in " __FILE__)
	@endcode  */
	#define OEL_ABORT(msg) (std::abort(), (void) msg)

	#if !defined OEL_ASSERT && !defined NDEBUG
	#include <cassert>

	//! Can be defined to your own
	#define OEL_ASSERT  assert
	#endif
#endif



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


#if OEL_MEM_BOUND_DEBUG_LVL == 0
	#undef OEL_ASSERT
	#define OEL_ASSERT(expr) ((void) 0)
#elif !defined OEL_ASSERT
	#define OEL_ASSERT(expr)  \
		((expr) || (OEL_ABORT("Failed assert " #expr), false))
#endif



#if defined _CPPUNWIND || defined __EXCEPTIONS
	#define OEL_THROW(exception, msg) throw exception
	#define OEL_TRY_                  try
	#define OEL_CATCH_ALL             catch (...)
	#define OEL_WHEN_EXCEPTIONS_ON(x) x
#else
	#define OEL_THROW(exc, message)   OEL_ABORT(message)
	#define OEL_TRY_
	#define OEL_CATCH_ALL             OEL_CONST_COND if (false)
	#define OEL_WHEN_EXCEPTIONS_ON(x)
#endif
