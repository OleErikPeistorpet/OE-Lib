#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"

/** @file
*/

namespace oel::view
{

//! A minimal substitute for std::ranges::subrange and boost::iterator_range
template< typename Iterator, typename Sentinel >
class subrange
{
	_detail::TightPair<Iterator, Sentinel> _m;

public:
	using difference_type = iter_difference_t<Iterator>;

	subrange() = default;
	constexpr subrange(Iterator first, Sentinel last)   : _m{std::move(first), last} {}

	constexpr Iterator begin()       { return _detail::MoveIfNotCopyable(_m.first); }

	constexpr Sentinel end() const   { return _m.second(); }

	//! Provided only if begin() can be subtracted from end()
	template< typename I = Iterator,
	          enable_if< !disable_sized_sentinel_for<Sentinel, I> > = 0,
	          typename Ret = decltype( as_unsigned(std::declval<Sentinel>() - std::declval<I>()) )
	>
	constexpr Ret  size() const
		{
			return static_cast<Ret>(_m.second() - _m.first);
		}

	constexpr bool empty() const   { return _m.first == _m.second(); }

	constexpr decltype(auto) operator[](difference_type index) const
		OEL_REQUIRES(iter_is_random_access<Iterator>)          OEL_ALWAYS_INLINE { return _m.first[index]; }
};

}


#if OEL_STD_RANGES

namespace std::ranges
{

template< typename I, typename S >
inline constexpr bool enable_borrowed_range< oel::view::subrange<I, S> > = true;

template< typename I, typename S >
inline constexpr bool enable_view< oel::view::subrange<I, S> > = true;

}
#endif
