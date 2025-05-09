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

		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			if constexpr (std::is_lvalue_reference_v<Range>)
			{
				if constexpr (range_is_sized<Range>)
					return view::counted(begin(r), oel::ssize(r));
				else
					return view::subrange(begin(r), end(r));
			}
			else
			{	return view::owning(static_cast<Range &&>(r));
			}
		}
	};
}


//! View types and view creation functions. The API tries to mimic views in standard `<ranges>`
namespace view
{
//! Substitute for std::views::all
inline constexpr _detail::All all;
}

} // oel
