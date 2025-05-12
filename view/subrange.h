#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h" // for as_unsigned

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
	using difference_type = iter_difference_t<Iterator>;

	constexpr subrange(Iterator first, Sentinel last)   : _begin(std::move(first)), _end(last) {}

	constexpr Iterator begin()       { return _detail::MoveIfNotCopyable(_begin); }

	constexpr Sentinel end() const   { return _end; }

	//! Provided only if `begin()` can be subtracted from `end()`
	template
	<	typename I = Iterator,
		enable_if< !disable_sized_sentinel_for<Sentinel, I> > = 0,
		typename Ret = decltype( as_unsigned(std::declval<Sentinel>() - std::declval<I>()) )
	>
	constexpr Ret  size() const
		{
			return static_cast<Ret>(_end - _begin);
		}

	constexpr bool empty() const   { return _begin == _end; }

	OEL_ALWAYS_INLINE
	constexpr decltype(auto) operator[](difference_type index) const
		OEL_REQUIRES(iter_is_random_access<Iterator>)              { return _begin[index]; }
};

}


template< typename I, typename S >
inline constexpr bool oel::enable_view< oel::view::subrange<I, S> > = true;

#if OEL_STD_RANGES

template< typename I, typename S >
inline constexpr bool std::ranges::enable_borrowed_range< oel::view::subrange<I, S> > = true;
#endif
