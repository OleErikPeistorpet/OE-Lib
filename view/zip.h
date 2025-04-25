#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "move.h"  // for iter_move
#include "zip_transform.h"

/** @file
*/

namespace oel
{
namespace _detail
{
	template< typename... Ts >
	struct ToTuple
	{
		constexpr std::tuple<Ts...> operator()(Ts... values) const
		{
			return{ static_cast<Ts>(values)... };
		}
	};

	template< typename... Iters, size_t... Ns >
	constexpr auto IterMoveTuple(const std::tuple<Iters...> & t, std::index_sequence<Ns...>)
	{
		using Ret = std::tuple< decltype( iter_move(std::get<Ns>(t)) )... >;
		return Ret{iter_move( std::get<Ns>(t) )...};
	}
}

////////////////////////////////////////////////////////////////////////////////

namespace view
{

//! Like std::views::zip, but all ranges are assumed to have the same size as the first
/**
* If any range has less elements than the first, using the resulting view has undefined behavior.
*
* https://en.cppreference.com/w/cpp/ranges/zip_view  */
inline constexpr auto zip =
	[](auto &&... ranges)
	{
		using F = _detail::ToTuple<decltype( *begin(ranges) )...>;
		return zip_transform( F{}, static_cast<decltype(ranges)>(ranges)... );
	};

} // view


namespace iter
{
template< typename... Ts, typename... Iterators >
constexpr auto iter_move(const _zipTransform< oel::_detail::ToTuple<Ts...>, Iterators... > & it)
	{
		return oel::_detail::IterMoveTuple(it.base(), std::index_sequence_for<Iterators...>{});
	}
}

}