#pragma once

// Copyright 2022 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "all.h"
#include "../auxi/zip_transform_iterator.h"

/** @file
*/

namespace oel::view
{

//! Almost same as std::views::zip_transform, but all ranges are assumed to have the same size as the first
/**
* If any range has less elements than the first, using the resulting view has undefined behavior.
*
* Unlike std::views::zip_transform, copies or moves the function into the iterator rather than
* storing it just in the view, thus saving one indirection when dereferencing the iterator.
*
* https://en.cppreference.com/w/cpp/ranges/zip_transform_view  */
inline constexpr auto zip_transform =
	[](auto func, auto &&... ranges)
	{
		using I = _zipTransformIterator< decltype(func), iterator_t<decltype(ranges)>... >;
		using V = _zipTransformView< I, decltype(func), decltype( all(static_cast<decltype(ranges)>(ranges)) )... >;
		return V{std::move(func), all( static_cast<decltype(ranges)>(ranges) )...};
	};

//! Almost same as `zip_transform(func, std::views::counted(iterators, count)...)`
/** @param count is the number of elements in the resulting view
 *  @param iterators are the beginning of each range to transform together
*/
inline constexpr auto zip_transform_n =
	[](auto func, auto count, auto... iterators)
	{
		return counted(
			_zipTransformIterator{std::move(func), std::move(iterators)...},
			count );
	};

}