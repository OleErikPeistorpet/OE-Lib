#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "iota.h"
#include "transform.h"
#include "unbounded.h"

/** @file
*/

namespace oel
{
namespace _detail
{
	template< typename Ref >
	struct IndexAndElemAsPair
	{
		template< typename I >
		constexpr auto operator()(I idx, Ref e) const
		{
			return std::pair<I, Ref>{idx, static_cast<Ref>(e)};
		}
	};
}

#if OEL_STD_RANGES
	template< typename Ref, typename... Is >
	constexpr auto iter_move(const transform_iterator< true, _detail::IndexAndElemAsPair<Ref>, Is... > & it)
		noexcept( noexcept(std::ranges::iter_move( std::get<1>(it.base()) ))
		          and std::is_nothrow_move_constructible_v<Ref> )
		{
			auto const   idx    = *std::get<0>(it.base());
			const auto & elemIt =  std::get<1>(it.base());
			return std::pair<decltype(idx), decltype( std::ranges::iter_move(elemIt) )>
					(idx, std::ranges::iter_move(elemIt));
		}
#endif


namespace view
{

struct _enumerateFn
{
	template< typename Range >
	friend constexpr auto operator |(Range && r, _enumerateFn)
		{
			using Idx = iter_difference_t< iterator_t<Range> >;
			using Ref = decltype(*begin(r));
			auto iv = unbounded(iota_iterator<Idx>{}); // lvalue to avoid view::all wrapping it with view::owning
			return zip_transform(_detail::IndexAndElemAsPair<Ref>{}, iv, static_cast<Range &&>(r));
		}

	template< typename Range >
	constexpr auto operator()(Range && r) const   { return static_cast<Range &&>(r) | *this; }
};
/** @brief Similar to std::views::enumerate (C++23)
*
* Note: Without C++20, view::move after enumerate will have no effect.
* To get elements as rvalues, move must come earlier in the chain like this: `range | view::move | view::enumerate` */
inline constexpr _enumerateFn enumerate;

}

} // namespace oel
