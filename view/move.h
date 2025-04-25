#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "transform.h"
#include "../auxi/contiguous_iterator_to_ptr.h"

/** @file
*/

namespace oel
{
namespace view
{

struct _moveFn
{
	template< typename InputRange >
	friend constexpr auto operator |(InputRange && r, _moveFn)
		{
			return _iterTransformView{oel_iter_move, all( static_cast<InputRange &&>(r) )};
		}

	template< typename InputRange >
	constexpr auto operator()(InputRange && r) const   { return static_cast<InputRange &&>(r) | *this; }
};
//! Very similar to views::move in the Range-v3 library and std::views::as_rvalue
/** @code
std::string moveFrom[2] {"abc", "def"};
oel::dynarray movedStrings(moveFrom | view::move);
@endcode  */
inline constexpr _moveFn move;

} // view



template< typename Iterator >
constexpr auto to_pointer_contiguous(_iterTransformIterator<_iterMove::Fn, Iterator> it) noexcept
->	decltype( to_pointer_contiguous(it.base()) )
	{  return to_pointer_contiguous(it.base()); }

}