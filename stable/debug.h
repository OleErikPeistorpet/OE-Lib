// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef OETL_HALT
#	if _MSC_VER
#		define OETL_HALT() __debugbreak()
#	else
#		define OETL_HALT() __asm__("int $3")
#	endif
#endif

#ifndef ASSERT_ALWAYS
/// The standard assert macro doesn't break on the line of the assert, so we roll our own
#	define ASSERT_ALWAYS(expr)  \
		do {  \
			if (!(expr)) OETL_HALT();  \
		} while((void) 0, false)
#endif

#if !defined(NDEBUG) && !defined(OETL_MEM_BOUND_DEBUG_LVL)
/** @brief Undefined: no array index and iterator checks. 1: fast checks. 2: most debug checks. 3: all checks, often slow.
*
* Warning: Levels 0 and 1 are not binary compatible with levels 2 and 3. */
#	define OETL_MEM_BOUND_DEBUG_LVL 3
#endif

#undef MEM_BOUND_ASSERT
#undef BOUND_ASSERT_CHEAP
#if OETL_MEM_BOUND_DEBUG_LVL >= 2
#	define MEM_BOUND_ASSERT ASSERT_ALWAYS
#else
#	define MEM_BOUND_ASSERT(expr) ((void) 0)
#endif
#if OETL_MEM_BOUND_DEBUG_LVL
#	define BOUND_ASSERT_CHEAP ASSERT_ALWAYS
#else
#	define BOUND_ASSERT_CHEAP(expr) ((void) 0)
#endif
