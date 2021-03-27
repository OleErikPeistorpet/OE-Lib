#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail/all.h"

/** @file
*/

namespace oel
{
namespace view
{

/** @brief Create a view with transform_iterator from a range
@code
std::bitset<8> arr[] { 3, 5, 7, 11 };
dynarray<std::string> result;
result.append( view::transform(arr, [](const auto & bs) { return bs.to_string(); }) );
@endcode
* Similar to boost::adaptors::transform, but stores just one copy of f and has no size overhead for stateless
* function objects. Also accepts a lambda as long as any by-value captures are trivially copy constructible
* and trivially destructible. Moreover, UnaryFunc can have non-const operator() (such as mutable lambda). <br>
* Note that passing an rvalue range is meant to give a compile error. Use a named variable. */
template< typename UnaryFunc, typename Range >
constexpr auto transform(Range & r, UnaryFunc f)
	{
		using I = decltype(begin(r));
		return _detail::Transf<UnaryFunc, I>::call(_detail::All<I>(r), f);
	}
} // view

}