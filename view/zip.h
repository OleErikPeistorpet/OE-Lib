#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "zip_transform.h"

/** @file
*/

namespace oel::view
{

//! Like std::views::zip, but all ranges are assumed to have the same size as the first
/**
* If any range has less elements than the first, using the resulting view has undefined behavior.
*
* https://en.cppreference.com/w/cpp/ranges/zip_view  */
inline constexpr auto zip =
	[](auto &&... ranges)
	{
		using F = _detail::ToTuple<decltype( *begin(ranges) )...>;
		return zip_transform( F{}, static_cast<decltype(ranges)>(ranges)... );
	};

}