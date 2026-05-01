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
		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			using R = std::remove_cv_t< std::remove_reference_t<Range> >;
			if constexpr( OEL_NS_OF_ENABLE_VIEW::enable_view<R> and std::is_move_constructible_v<R> )
			{
				return static_cast<Range &&>(r);
			}
		#if OEL_STD_RANGES
			else if constexpr(
				requires{ std::ranges::ref_view{static_cast<Range &&>(r)}; } )
			{	return    std::ranges::ref_view{static_cast<Range &&>(r)};
			}
		#endif
			else if constexpr( std::is_lvalue_reference_v<Range> )
			{
				if constexpr( range_is_sized<Range> )
					return view::counted(begin(r), oel::ssize(r));
				else
					return view::subrange(begin(r), end(r));
			}
			else
			{	return view::owning( static_cast<Range &&>(r) );
			}
		}
	};
}


//! View types and view creation functions. The API tries to mimic views in standard `<ranges>`
namespace view
{
//! Very similar to std::views::all
inline constexpr _detail::All all;
}

} // oel
