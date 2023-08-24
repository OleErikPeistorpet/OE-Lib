#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"  // for as_unsigned
#include "detail/misc.h"

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
	constexpr basic_view(Iterator f, Sentinel l)   : _begin{std::move(f)}, _end{l} {}

	constexpr Iterator begin()       { return _detail::MoveIfNotCopyable(_begin); }

	constexpr Sentinel end() const   OEL_ALWAYS_INLINE { return _end; }

	//! Provided only if begin() can be subtracted from end()
	template< typename I = Iterator,
	          enable_if< !disable_sized_sentinel_for<Sentinel, I> > = 0
	>
	constexpr auto size() const
	->	decltype( as_unsigned(std::declval<Sentinel>() - std::declval<I>()) )  { return _end - _begin; }

	constexpr bool empty() const   { return _begin == _end; }

	constexpr decltype(auto) operator[](difference_type index) const   OEL_ALWAYS_INLINE
		OEL_REQUIRES(iter_is_random_access<Iterator>)          { return _begin[index]; }

protected:
	Iterator _begin;
	Sentinel _end;
};

//! View creation functions. Trying to mimic subset of C++20 ranges
namespace view
{

//! Create a basic_view from iterator pair, or iterator and sentinel
inline constexpr auto subrange =
	[](auto first, auto last) { return basic_view{std::move(first), last}; };

}

} // oel

#if OEL_STD_RANGES

template< typename I, typename S >
inline constexpr bool std::ranges::enable_borrowed_range< oel::basic_view<I, S> > = true;

template< typename I, typename S >
inline constexpr bool std::ranges::enable_view< oel::basic_view<I, S> > = true;

#endif
