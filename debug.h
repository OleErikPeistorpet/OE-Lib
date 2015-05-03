// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef OEL_CONST_COND
	#if _MSC_VER
		#define OEL_CONST_COND __pragma(warning(suppress : 4127))
	#else
		#define OEL_CONST_COND
	#endif
#endif


#ifndef OEL_HALT
	#if _MSC_VER
		#define OEL_HALT() __debugbreak()
	#else
		#define OEL_HALT() __asm__("int $3")
	#endif
#endif

#ifndef ASSERT_ALWAYS
	/// The standard assert macro rarely breaks on the line of the assert, so we roll our own
	#define ASSERT_ALWAYS(expr)  \
		OEL_CONST_COND  \
		do {  \
			if (!(expr)) OEL_HALT();  \
		} while (false)
#endif

#if !defined(NDEBUG) && !defined(OEL_MEM_BOUND_DEBUG_LVL)
/** @brief Undefined: no array index and iterator checks. 1: fast checks. 2: most debug checks. 3: all checks, often slow.
*
* Warning: Levels 0 and 1 are not binary compatible with levels 2 and 3. */
	#define OEL_MEM_BOUND_DEBUG_LVL 3
#endif

#undef OEL_MEM_BOUND_ASSERT
#undef MEM_BOUND_ASSERT_CHEAP
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	#define OEL_MEM_BOUND_ASSERT ASSERT_ALWAYS
#else
	#define OEL_MEM_BOUND_ASSERT(expr) ((void) 0)
#endif
#if OEL_MEM_BOUND_DEBUG_LVL
	#define MEM_BOUND_ASSERT_CHEAP ASSERT_ALWAYS
#else
	#define MEM_BOUND_ASSERT_CHEAP(expr) ((void) 0)
#endif
