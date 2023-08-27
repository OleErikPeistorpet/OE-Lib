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

namespace oel
{
namespace _detail
{
	struct All
	{
	#if OEL_STD_RANGES
		template< std::ranges::viewable_range R >
		constexpr auto operator()(R && r) const
		{
			return std::views::all(static_cast<R &&>(r));
		}
	#endif

		template< typename SizedRange >
		constexpr auto operator()(SizedRange & r) const
		->	decltype( view::counted(begin(r), oel::ssize(r)) )
		{	return    view::counted(begin(r), oel::ssize(r)); }

		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			if constexpr (std::is_lvalue_reference_v<Range>)
				return view::subrange(begin(r), end(r));
			else
				return view::owning(static_cast<Range &&>(r));
		}
	};
}


//! View types and view creation functions. Trying to mimic subset of C++20 ranges
namespace view
{
//! Substitute for std::views::all
inline constexpr _detail::All all;
}

} // oel
