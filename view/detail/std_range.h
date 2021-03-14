#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifdef __has_include
#if __cpp_lib_ranges or (__has_include(<ranges>) and _HAS_CXX20)
	#include <ranges>

	#define OEL_HAS_RANGES  1
#endif
#endif
