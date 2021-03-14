#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../../fwd.h"

//! Users may define (to override __cpp_lib_ranges or to not pay for include)
#ifndef OEL_STD_RANGES
	#if __cpp_lib_ranges < 201911
	#define OEL_STD_RANGES  0
	#else
	#define OEL_STD_RANGES  1
	#endif
#endif

#if OEL_STD_RANGES
#include <ranges>
#endif
