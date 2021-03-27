#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../auxi/type_traits.h"
#include "detail/core.h"

/** @file
*/

namespace oel
{

//! A minimal substitute for boost::iterator_range and std::ranges::subrange (C++20)
template< typename Iterator, typename Sentinel = Iterator >
class basic_view
{
public:
	using difference_type = iter_difference_t<Iterator>;

	basic_view() = default;
	constexpr basic_view(Iterator f, Sentinel l)  : _begin(f), _end(l) {}

	constexpr Iterator begin() const   OEL_ALWAYS_INLINE { return _begin; }
	constexpr Sentinel end() const     OEL_ALWAYS_INLINE { return _end; }

protected:
	Iterator _begin;
	Sentinel _end;
};

//! View creation functions. Trying to mimic subset of C++20 ranges
namespace view
{

//! Create a basic_view from iterator pair, or iterator and sentinel
template< typename Iterator, typename Sentinel >
constexpr basic_view<Iterator, Sentinel> subrange(Iterator first, Sentinel last)  { return {first, last}; }

}

} // oel

#if OEL_STD_RANGES

template< typename I, typename S >
inline constexpr bool std::ranges::enable_borrowed_range< oel::basic_view<I, S> > = true;

template< typename I, typename S >
inline constexpr bool std::ranges::enable_view< oel::basic_view<I, S> > = true;

#endif
