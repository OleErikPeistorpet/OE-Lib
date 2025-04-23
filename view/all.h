#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "owning.h"

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
			if constexpr( ranges::enable_view< std::remove_cvref_t<Range> > )
			{
				return static_cast<Range &&>(r);
			}
			else if constexpr(
				requires{ ranges::ref_view{static_cast<Range &&>(r)}; } )
			{	return    ranges::ref_view{static_cast<Range &&>(r)};
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

//! Nearly same as std::views::all
inline constexpr _detail::All all;

//! Nearly same as std::views::all_t
template< typename Range >
using all_t = decltype( all(std::declval<Range>()) );

} // view

}