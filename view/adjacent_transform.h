#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "counted.h"
#include "transform.h"

/** @file
*/

namespace oel
{
namespace view
{

template< ptrdiff_t N >
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
* Requires that the range is random-access and borrowed (lvalue or std::ranges::enable_borrowed_range is true).
*
* https://en.cppreference.com/w/cpp/ranges/adjacent_transform_view  */
template< ptrdiff_t N >
inline constexpr _adjacentTransformFn<N> adjacent_transform{};

} // view



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename F, ptrdiff_t N >
	struct AdjacTransPartial
	{
		F _f;

		static_assert(N > 0);

		using _fn_7KQw = F; // guarding against name collision due to inheritance (MSVC)

		struct _transform : public F
		{
			template< typename Iter, ::std::ptrdiff_t... Is >
			OEL_ALWAYS_INLINE constexpr auto apply_7KQw(Iter it, ::std::integer_sequence<::std::ptrdiff_t, Is...>) const
			{
				const _fn_7KQw & f = *this;
				return f(it[Is - N + 1]...);
			}

			template< typename Iter >
			constexpr decltype(auto) operator()(Iter it) const
			{
				return apply_7KQw(it, ::std::make_integer_sequence<::std::ptrdiff_t, N>{});
			}
		};

		template< typename Range >
		friend constexpr auto operator |(Range && r, AdjacTransPartial a)
		{
			static_assert(range_is_borrowed<Range>);

			auto const s = oel::ssize(r);
			auto  offset = N - 1;
			if (offset > s)
				offset = s;

			return _iterTransformView{
				view::counted(begin(r) + offset, s - offset),
				_transform{std::move(a)._f} };
		}

		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			return static_cast<Range &&>(r) | *this;
		}
	};
}

template< ptrdiff_t N >
template< typename Func >
constexpr auto view::_adjacentTransformFn<N>::operator()(Func f) const
{
	return _detail::AdjacTransPartial<Func, N>{std::move(f)};
}

} // oel
