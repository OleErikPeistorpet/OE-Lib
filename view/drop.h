#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail/may_sized.h"

/** @file
*/

namespace oel
{
namespace view
{

/** @brief Return a view consisting of all but the first count elements from the source range
* @pre The source r must have at least count elements
* @return Type counted_view if r.size() exists or r is an array, else basic_view.
*
* Note that passing an rvalue range is meant to give a compile error. Use a named variable. */
template< typename Range >
constexpr auto drop(Range & r, range_difference_t<Range> count)
	{
		return _detail::maySized(
			std::next(begin(r), count),
			r, count );
	}
} // view

}