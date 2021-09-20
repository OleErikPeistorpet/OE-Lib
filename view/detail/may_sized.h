#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../counted.h"
#include "../subrange.h"

namespace oel
{
namespace _detail
{
	template< typename Iterator, typename Range, typename... None >
	constexpr auto maySized(Iterator first, Range & r, iter_difference_t<Iterator> = 0, None...)
	{
		return basic_view<Iterator, decltype( end(r) )>
				{first, end(r)};
	}

	template< typename Iterator, typename SizedRange >
	constexpr auto maySized(Iterator first, SizedRange & r, iter_difference_t<Iterator> nDropped = 0)
	->	decltype( counted_view<Iterator>{first, oel::ssize(r) - nDropped} )
		 { return counted_view<Iterator>{first, oel::ssize(r) - nDropped}; }


	template< typename InputRange, typename... None >
	constexpr auto move(InputRange & r, None...)
	{
		using MI = std::move_iterator<decltype( begin(r) )>;
		return basic_view<MI>{ MI{begin(r)}, MovI{end(r)} };
	}

	template< typename SizedRange,
	          typename MI = std::move_iterator< iterator_t<SizedRange> >
	>
	constexpr auto move(SizedRange & r)
	->	decltype( counted_view<MI>{ MI{begin(r)}, oel::ssize(r) } )
		 { return counted_view<MI>{ MI{begin(r)}, oel::ssize(r) }; }
}