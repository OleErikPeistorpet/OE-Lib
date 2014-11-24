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
/// Undefined: no array index and iterator debug checks. 1: some debug checks (usually fast). 2: all checks.
#	define OETL_MEM_BOUND_DEBUG_LVL 2
#endif

#undef MEM_BOUND_ASSERT
#if OETL_MEM_BOUND_DEBUG_LVL
#	define MEM_BOUND_ASSERT ASSERT_ALWAYS
#else
#	define MEM_BOUND_ASSERT(expr) ((void) 0)
#endif
