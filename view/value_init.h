#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "generate.h"

/** @file
*/

namespace oel
{
namespace _detail
{
	template< typename T >
	struct ValueInit
	{
		constexpr T operator()() const { return T{}; }
	};
}


namespace view
{

template< typename T >
struct _valueInitFn
{
	constexpr auto operator()() const
		{
			return generate(_detail::ValueInit<T>{});
		}

	constexpr auto operator()(ptrdiff_t count) const
		{
			return generate(_detail::ValueInit<T>{}, count);
		}
};
//! `value_init<T>(count)` is like `std::views::repeat(T{}, count)`, but only for use in OE-Lib
/**
* `value_init<T>()` is unbounded like `std::views::repeat(T{})`.
* Suitable types get optimized by `memset` to zero when used with OE-Lib containers. */
template< typename T >
inline constexpr _valueInitFn<T> value_init;

} // view

}