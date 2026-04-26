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
	struct Repeat
	{
		T val;

		constexpr const T & operator()() const noexcept { return val; }
	};
}


namespace view
{

//! Like `std::views::repeat(value, count)`, but only for use in OE-Lib
/**
* https://en.cppreference.com/w/cpp/ranges/repeat_view  */
inline constexpr auto repeat =
	[](auto && value, ptrdiff_t count)
	{
		using R = decltype( value );
		return generate(
			_detail::Repeat< std::decay_t<R> >{static_cast<R>(value)},
			count );
	};

} // view

}