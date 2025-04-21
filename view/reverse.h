#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "all.h"

/** @file
*/

namespace oel
{

template< typename BidirectionalIter >  OEL_ALWAYS_INLINE
constexpr auto make_reverse_iterator(BidirectionalIter it)
	{
		return std::reverse_iterator<BidirectionalIter>{std::move(it)};
	}

template< typename Iter >
constexpr auto make_reverse_iterator(std::reverse_iterator<Iter> it)
	{
		return it.base();
	}


template< typename View >
class _reverseView
{
	View _v;

public:
	using difference_type = iter_difference_t< iterator_t<View> >;

	constexpr explicit _reverseView(View && v)   : _v{std::move(v)} {}

	constexpr auto begin()
		{
			return oel::make_reverse_iterator(_v.end());
		}
	constexpr auto end()
		{
			return oel::make_reverse_iterator(_v.begin());
		}

	template< typename V = View >  OEL_ALWAYS_INLINE
	constexpr auto size()
	->	decltype( std::declval<V>().size() )  { return _v.size(); }

	constexpr bool empty()   { return _v.empty(); }

	constexpr decltype(auto) operator[](difference_type index)
		OEL_REQUIRES(iter_is_random_access< sentinel_t<View> >)
		{
			return _v.end()[-1 - index];
		}

	constexpr View         base() &&                { return std::move(_v); }
	constexpr const View & base() const & noexcept  { return _v; }
};

namespace view
{

struct _reverseFn
{
	template< typename BidirectionalRange >
	friend constexpr auto operator |(BidirectionalRange && r, _reverseFn)
		{
			auto v = all( static_cast<BidirectionalRange &&>(r) );
			return _reverseView< decltype(v) >{std::move(v)};
		}

	template< typename BidirectionalRange >
	constexpr auto operator()(BidirectionalRange && r) const
		{
			return static_cast<BidirectionalRange &&>(r) | _reverseFn{};
		}
};
//! Very similar to std::views::reverse
inline constexpr _reverseFn reverse;

}

} // oel


template< typename V >
inline constexpr bool oel::enable_view< oel::_reverseView<V> > = true;

#if OEL_STD_RANGES

template< typename V >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_reverseView<V> >
= enable_borrowed_range< std::remove_cv_t<V> >;
#endif
