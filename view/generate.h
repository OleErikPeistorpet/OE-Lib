#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "counted.h"
#include "../auxi/assignable.h"

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
	using difference_type   = ptrdiff_t;
	using reference         = decltype( std::declval<Generator &>()() );
	using pointer           = void;
	using value_type        = std::remove_cv_t< std::remove_reference_t<reference> >;

	constexpr explicit _generateIterator(Generator g)   : _g{std::move(g)} {}

	constexpr reference operator*()
		{
			return static_cast<Generator &>(_g)();
		}

	constexpr _generateIterator & operator++()   OEL_ALWAYS_INLINE { return *this; }
};


namespace view
{
//! TODO
inline constexpr auto generate_n =
	[](auto generator, ptrdiff_t count)  { return counted(_generateIterator{std::move(generator)}, count); };
}

} // oel
