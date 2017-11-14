#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/macros.h"

#include <stdexcept>


/** @file
*/

#if !defined(NDEBUG) && !defined(OEL_MEM_BOUND_DEBUG_LVL)
/** @brief Undefined/0: no array index and iterator checks. 1: most debug checks. 2: all checks, often slow.
*
* 0 (or undefined by NDEBUG defined) is not binary compatible with levels 1 and 2. */
	#define OEL_MEM_BOUND_DEBUG_LVL 2
#endif

#ifndef OEL_ALWAYS_ASSERT
	#ifndef OEL_HALT
	/** Could throw an exception instead, or do whatever. If it contains assembly, compilation will likely fail.
	*
	* Example: @code
	#define OEL_HALT(failedCond) throw std::logic_error(failedCond ", assertion failed in " __FILE__)
	@endcode  */
		#if defined(_MSC_VER)
		#define OEL_HALT(failedCond) __debugbreak()
		#elif OEL_HAS_BUILTIN_TRAP
		#define OEL_HALT(failedCond) __builtin_trap()
		#else
		#define OEL_HALT(failedCond) std::abort()
		#endif
	#endif

	//! Executes OEL_HALT on failure. Can be defined to standard assert or your own
	#define OEL_ALWAYS_ASSERT(expr)  \
		OEL_CONST_COND  \
		do {  \
			if (!(expr)) OEL_HALT(#expr);  \
		} while (false)
#endif



////////////////////////////////////////////////////////////////////////////////



#undef OEL_HAS_BUILTIN_TRAP

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


namespace oel
{
namespace _detail
{
	struct Throw
	{	// at namespace scope this produces warnings of unreferenced function or failed inlining
	#ifdef _MSC_VER
		__declspec(noreturn)
	#else
		__attribute__((noreturn))
	#endif
		static void OutOfRange(const char * what)
		{
			OEL_THROW(std::out_of_range(what));
			(void) what; // avoid warning when exceptions disabled
		}
	};
}

}