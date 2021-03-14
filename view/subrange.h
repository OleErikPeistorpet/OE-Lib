#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail/std_range.h"
#include "../algo/type_traits.h"

/** @file
*/

namespace oel
{

//! A minimal substitute for boost::iterator_range and std::ranges::subrange (C++20)
template< typename Iterator, typename Sentinel = Iterator >
class basic_view
	#if OEL_HAS_RANGES
	 :	public std::ranges::view_base
	#endif
{
public:
	basic_view() = default;
	constexpr basic_view(Iterator f, Sentinel l)  : _begin(f), _end(l) {}

	constexpr Iterator begin() const   OEL_ALWAYS_INLINE { return _begin; }
	constexpr Sentinel end() const     OEL_ALWAYS_INLINE { return _end; }

protected:
	Iterator _begin;
	Sentinel _end;
};

template< typename Iterator,
          enable_if< iter_is_random_access<Iterator>::value > = 0
>
constexpr iter_difference_t<Iterator> ssize(const basic_view<Iterator> & r)   { return r.end() - r.begin(); }

//! View creation functions. Trying to mimic subset of C++20 ranges
namespace view
{

//! Create a basic_view from two iterators, with type deduced from arguments
template< typename Iterator >
constexpr basic_view<Iterator> subrange(Iterator first, Iterator last)  { return {first, last}; }

}

} // oel
