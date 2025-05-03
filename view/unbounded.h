#pragma once

// Copyright 2022 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../auxi/range_traits.h"

/** @file
*/

namespace oel::view
{

//! TODO
template< typename Iterator >
class unbounded
{
	Iterator _begin;

public:
	using difference_type = iter_difference_t<Iterator>;

	unbounded() = default;
	constexpr explicit unbounded(Iterator first)   : _begin{std::move(first)} {}

	constexpr Iterator begin()    { return _detail::MoveIfNotCopyable(_begin); }

	static constexpr unreachable_sentinel_t end()  { return {}; }
};

}


#if OEL_STD_RANGES

namespace std::ranges
{

template< typename I >
inline constexpr bool enable_borrowed_range< oel::view::unbounded<I> > = true;

template< typename I >
inline constexpr bool enable_view< oel::view::unbounded<I> > = true;

}
#endif
