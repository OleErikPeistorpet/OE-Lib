#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../auxi/range_traits.h"

/** @file
*/

namespace oel::view
{

//! A substitute for std::ranges::subrange
/**
* If Sentinel is an empty type, `sizeof(subrange)` is equal to `sizeof(Iterator)`, unlike some standard implementations. */
template< typename Iterator, typename Sentinel >
class subrange
{
	Iterator _begin;
	OEL_NO_UNIQUE_ADDRESS Sentinel _end;

public:
	using difference_type = std::iter_difference_t<Iterator>;

	constexpr subrange(Iterator first, Sentinel last)   : _begin(std::move(first)), _end(last) {}

	constexpr Iterator begin()       { return _detail::MoveIfNotCopyable(_begin); }

	constexpr Sentinel end() const   { return _end; }

	constexpr auto size() const
		requires std::sized_sentinel_for<Sentinel, Iterator>
		{
			return static_cast< std::make_unsigned_t<difference_type> >(_end - _begin);
		}

	constexpr bool empty() const   { return _begin == _end; }

	OEL_ALWAYS_INLINE
	constexpr decltype(auto) operator[](difference_type index) const
		requires iter_is_random_access<Iterator>                   { return _begin[index]; }
};

}


namespace std::ranges
{

template< typename I, typename S >
inline constexpr bool enable_view< oel::view::subrange<I, S> > = true;

template< typename I, typename S >
inline constexpr bool enable_borrowed_range< oel::view::subrange<I, S> > = true;

}