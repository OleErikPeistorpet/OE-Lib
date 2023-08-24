#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "counted.h"
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

		template< typename I, typename S >
		constexpr auto operator()(basic_view<I, S> v) const { return v; }

		template< typename I >
		constexpr auto operator()(counted_view<I> v) const { return v; }

		template< typename SizedRange >
		constexpr auto operator()(SizedRange & r) const
		->	decltype( view::counted(begin(r), oel::ssize(r)) )
		{	return    view::counted(begin(r), oel::ssize(r)); }

		template< typename Range, typename... None >
		constexpr auto operator()(Range & r, None...) const
		{
			return view::subrange(begin(r), end(r));
		}

		template< typename R >
		void operator()(R &&) const = delete;
	};
}


//! View creation functions. The API tries to mimic views in std::ranges
namespace view
{
//! Substitute for std::views::all
inline constexpr _detail::All all;
}

} // oel
