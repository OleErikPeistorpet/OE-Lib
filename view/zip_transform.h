#pragma once

// Copyright 2022 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "counted.h"
#include "../auxi/zip_transform_iterator.h"

/** @file
*/

namespace oel::view
{
//! Almost same as `std::views::zip_transform(func, view::counted(iterators, count)...)`
/** @param count is the number of elements in the resulting view
 *  @param iterators are the beginning of each range to transform together
*
* Unlike std::views::zip_transform, copies or moves the function into the iterator rather than
* storing it just in the view, thus saving one indirection when dereferencing the iterator.
*
* https://en.cppreference.com/w/cpp/ranges/zip_transform_view  */
inline constexpr auto zip_transform_n =
	[](auto func, auto count, auto... iterators)
	{
		return counted(_zipTransformIterator{ std::move(func), std::move(iterators)... }, count);
	};

}