#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "generate.h"

/** @file
*/

namespace oel::view
{

//! Similar to `std::views::iota(beginVal, endVal)`
inline constexpr auto iota =
	[](auto beginVal, auto endVal)
	{
		return generate_n(
			[beginVal]() mutable { return beginVal++; },
			endVal - beginVal );
	};

}