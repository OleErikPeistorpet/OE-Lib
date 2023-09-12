#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "zip_transform.h"

/** @file
*/

namespace oel
{
namespace view
{

template< size_t N >
struct _adjacentTransformFn
{
	//! Used with operator |
	template< typename Func >
	constexpr auto operator()(Func f) const;

	template< typename Range, typename Func >
	constexpr auto operator()(Range && r, Func f) const
		{
			return static_cast<Range &&>(r) | (*this)(std::move(f));
		}
};
//! Similar to std::views::adjacent_transform, same call signature
/**
* Function objects are required to have const `operator()`, unlike std::views::adjacent_transform.
* This is because the function is copied or moved into the iterator rather than storing it
* just in the view, in order to save one indirection when dereferencing the iterator.
* https://en.cppreference.com/w/cpp/ranges/adjacent_transform_view  */
template< size_t N >
inline constexpr _adjacentTransformFn<N> adjacent_transform{};

} // view



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename F, size_t N >
	struct AdjacTransPartial
	{
		F _f;

		template< typename Range, size_t... Is >
		constexpr auto _make(Range & r, std::index_sequence<Is...>)
		{
			auto const n = oel::ssize(r) - ptrdiff_t{N - 1};
			auto      it = begin(r);
			auto const l = end(r);
			return view::zip_transform_n
			{	std::move(_f),
				std::max(n, decltype(n){}),
				it,
				(Is, it != l ? ++it : it)...
			}; // brace init forces evaluation order
		}

		template< typename Range >
		friend constexpr auto operator |(Range && r, AdjacTransPartial a)
		{
			static_assert(range_is_borrowed<Range>);
			return a._make(r, std::make_index_sequence<N - 1>{});
		}

		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			return static_cast<Range &&>(r) | *this;
		}
	};
}

////////////////////////////////////////////////////////////////////////////////

template< size_t N >
template< typename Func >
constexpr auto view::_adjacentTransformFn<N>::operator()(Func f) const
{
	return _detail::AdjacTransPartial<Func, N>{std::move(f)};
}

} // oel
