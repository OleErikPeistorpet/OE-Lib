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

template< typename View >
class _reverseView
{
	View _base;

public:
	using difference_type = iter_difference_t< iterator_t<View> >;

	_reverseView() = default;
	constexpr explicit _reverseView(View v)   : _base{std::move(v)} {}

	constexpr auto begin()
		{
			return std::make_reverse_iterator(_base.end());
		}
	constexpr auto end()
		{
			return std::make_reverse_iterator(_base.begin());
		}

	template< typename V = View >  OEL_ALWAYS_INLINE
	constexpr auto size()
	->	decltype( std::declval<V>().size() )  { return _base.size(); }

	constexpr bool empty()   { return _base.empty(); }

	constexpr decltype(auto) operator[](difference_type index)
		OEL_REQUIRES(iter_is_random_access< sentinel_t<View> >)
		{
			return _base.end()[-1 - index];
		}

	constexpr View         base() &&                { return std::move(_base); }
	constexpr const View & base() const & noexcept  { return _base; }
};

namespace view
{

struct _reverseFn
{
	template< typename InputRange >
	constexpr auto operator()(InputRange && r) const
		{
			return _reverseView{all( static_cast<InputRange &&>(r) )};
		}
	template< typename V >
	constexpr auto operator()(_reverseView<V> r) const
		{
			return std::move(r).base();
		}

	template< typename InputRange >
	friend constexpr auto operator |(InputRange && r, _reverseFn f)
		{
			return f(static_cast<InputRange &&>(r));
		}
};
//! Very similar to std::views::reverse
inline constexpr _reverseFn reverse;

}

} // oel


#if OEL_STD_RANGES

namespace std::ranges
{

template< typename V >
inline constexpr bool enable_borrowed_range< oel::_reverseView<V> >
	= enable_borrowed_range< std::remove_cv_t<V> >;

template< typename V >
inline constexpr bool enable_view< oel::_reverseView<V> > = true;

}
#endif
