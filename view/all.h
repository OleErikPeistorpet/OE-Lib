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
namespace view
{
#if OEL_STD_RANGES
	template< std::ranges::viewable_range R >
	constexpr auto all(R && r)
		{
			return std::views::all(static_cast<R &&>(r));
		}
#endif

template< typename I, typename S >
constexpr auto all(basic_view<I, S> v)  { return v; }

template< typename I >
constexpr auto all(counted_view<I> v)   { return v; }

template< typename SizedRange >
constexpr auto all(SizedRange & r)
->	decltype( view::counted(begin(r), oel::ssize(r)) )
	 { return view::counted(begin(r), oel::ssize(r)); }

template< typename Range, typename... None >
constexpr auto all(Range & r, None...)
	{
		return view::subrange(begin(r), end(r));
	}

template< typename R >
constexpr void all(R &&) = delete;

}

} // oel
