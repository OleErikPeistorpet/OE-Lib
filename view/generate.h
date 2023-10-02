#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "counted.h"
#include "../auxi/detail_assignable.h"

/** @file
*/

namespace oel
{

template< typename Generator >
class _generateIterator
{
	typename _detail::AssignableWrap<Generator>::Type _g;

public:
	using iterator_category = std::input_iterator_tag;
	using reference         = decltype( std::declval<Generator &>()() );
	using pointer           = void;
	using value_type        = std::remove_cv_t< std::remove_reference_t<reference> >;
	using difference_type   = ptrdiff_t;

	constexpr explicit _generateIterator(Generator g)   : _g{std::move(g)} {}

	constexpr reference operator*()
		{
			return static_cast<Generator &>(_g)();
		}

	OEL_ALWAYS_INLINE
	constexpr _generateIterator & operator++()       { return *this; }
	OEL_ALWAYS_INLINE
	constexpr void                operator++(int) &  {}
};


namespace view
{
//! Returns a view that generates `count` elements by calling the given generator function
/**
* Like `generate_n` in the Range-v3 library, but this is only for use within OE-Lib. */
inline constexpr auto generate =
	[](auto generator, ptrdiff_t count)  { return counted(_generateIterator{std::move(generator)}, count); };
}

} // oel
