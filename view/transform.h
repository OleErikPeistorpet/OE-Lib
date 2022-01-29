#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "zip_transform.h"

/** @file
*/

namespace oel
{
namespace _detail
{
	template< typename F >
	struct TransfPartial
	{
		F _f;

		template< typename Range >
		friend constexpr auto operator |(Range && r, TransfPartial t)
		{
			using V = _transformView< false, F, decltype(view::all( static_cast<Range &&>(r) )) >;
			return V{std::move(t)._f, view::all( static_cast<Range &&>(r) )};
		}

		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			return static_cast<Range &&>(r) | *this;
		}
	};
}


namespace view
{

struct _transformFn
{
	//! Used with operator |
	template< typename UnaryFunc >
	constexpr auto operator()(UnaryFunc f) const   { return _detail::TransfPartial<UnaryFunc>{std::move(f)}; }

	template< typename Range, typename UnaryFunc >
	constexpr auto operator()(Range && r, UnaryFunc f) const
		{
			return static_cast<Range &&>(r) | (*this)(std::move(f));
		}
};
/**
* @brief Similar to std::views::transform, same call signature
*
* Function objects are required to have const `operator()`, unlike std::views::transform.
* This is because the function is copied or moved into the iterator rather than storing it
* just in the view, in order to save one indirection when dereferencing the iterator.
* https://en.cppreference.com/w/cpp/ranges/transform_view  */
inline constexpr _transformFn transform;

} // view

}