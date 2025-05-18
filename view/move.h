#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "transform.h"
#include "../auxi/iter_as_contiguous_address.h"

/** @file
*/

namespace oel
{
namespace _iterMove
{
	template< typename >
	void iter_move() = delete;

	struct Fn
	{
		template< typename I >
		constexpr auto operator()(I && it) const
			noexcept(noexcept( iter_move(it) ))
		->	decltype(          iter_move(it) )
			{        return    iter_move(it); }

		template< typename I, typename... None >
		constexpr decltype(auto) operator()(I && it, None...) const
			noexcept(noexcept(*it))
		{
			using T = decltype(*it);
			if constexpr( std::is_lvalue_reference_v<T> )
				return static_cast< std::remove_reference_t<T> && >(*it);
			else
				return *it;
        }
	};
}

//! Effectively same as std::ranges::iter_move
inline constexpr _iterMove::Fn iter_move;


namespace view
{

struct _moveFn
{
	template< typename InputRange >
	friend constexpr auto operator |(InputRange && r, _moveFn)
		{
			return _iterTransformView{iter_move, all( static_cast<InputRange &&>(r) )};
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


namespace iter
{

template< typename Iterator >
constexpr decltype(auto) iter_move(const _iterTransform<_iterMove::Fn, Iterator> & it)
	noexcept(noexcept( oel::iter_move(it.base()) ))
	{        return    oel::iter_move(it.base()); }

// TODO: _iterTransform with _iterMove::Fn should model std::contiguous_iterator

template< typename Iterator >
constexpr auto as_contiguous_address(_iterTransform<_iterMove::Fn, Iterator> it) noexcept
->	decltype( as_contiguous_address(it.base()) )
	{  return as_contiguous_address(it.base()); }

}

}