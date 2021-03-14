#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail/cpy.h"
#include "detail/throw.h"
#include "type_traits.h"

/** @file
* @brief Range-based copy functions, mostly intended to copy or move into arrays
*/

namespace oel
{

template< typename Iterator >
struct copy_return
{
	Iterator source_last;
};
/**
* @brief Copies the elements in source into the range beginning at dest
* @return `begin(source)` incremented by source size
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that source has size() member or is an array, and that dest models RandomAccessIterator.
* To move instead of copy, pass `view::move(source)`. To mimic std::copy_n, use view::counted.
* (Views can be used for all functions taking a range as source)  */
template< typename SizedInputRange, typename RandomAccessIter >  inline
auto copy_unsafe(SizedInputRange && source, RandomAccessIter dest)
->	copy_return<decltype(begin(source))>
	                                     { return {_detail::CopyUnsf(begin(source), oel::ssize(source), dest)}; }
/**
* @brief Copies the elements in source range into dest range, throws std::out_of_range if dest is smaller than source
* @return `begin(source)` incremented by the number of elements in source
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that source has size() member or is an array, and dest is a random_access_range (C++20 concept)  */
template< typename SizedInputRange, typename RandomAccessRange >
auto copy(SizedInputRange && source, RandomAccessRange && dest)
->	copy_return<decltype(begin(source))>;
/**
* @brief Copies as many elements from source as will fit in dest
* @return true if all elements were copied, false means truncation happened
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that dest is a random_access_range (otherwise compilation will fail)  */
template< typename InputRange, typename RandomAccessRange >  inline
bool copy_fit(InputRange && source, RandomAccessRange && dest)   { return _detail::CopyFit(source, dest); }

} // oel



////////////////////////////////////////////////////////////////////////////////
//
// Only implementation


template< typename SizedInputRange, typename RandomAccessRange >
auto oel::copy(SizedInputRange && src, RandomAccessRange && dest)
->	copy_return<decltype(begin(src))>
{
	if (oel::ssize(src) <= oel::ssize(dest))
		return oel::copy_unsafe(src, begin(dest));
	else
		_detail::Throw::outOfRange("Too small dest oel::copy");
}