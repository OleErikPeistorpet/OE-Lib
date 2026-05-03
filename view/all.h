#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "counted.h"
#include "owning.h"
#include "subrange.h"

/** @file
*/

namespace oel::view
{
//! Very similar to std::views::all
inline constexpr auto all =
	[](auto && range)
	{
		using Ref = decltype(range);
		using UnCVR = std::remove_cv_t< std::remove_reference_t<Ref> >;
		if constexpr( OEL_NS_OF_ENABLE_VIEW::enable_view<UnCVR> and std::is_move_constructible_v<UnCVR> )
		{
			return static_cast<Ref>(range);
		}
	#if OEL_STD_RANGES
		else if constexpr(
			requires{ std::ranges::ref_view{static_cast<Ref>(range)}; } )
		{	return    std::ranges::ref_view{static_cast<Ref>(range)};
		}
	#endif
		else if constexpr( std::is_lvalue_reference_v<Ref> )
		{
			if constexpr( range_is_sized<Ref> )
				return view::counted(begin(range), oel::ssize(range));
			else
				return view::subrange(begin(range), end(range));
		}
		else
		{	return view::owning( static_cast<Ref>(range) );
		}
	};

}