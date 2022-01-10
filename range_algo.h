#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/range_algo_detail.h"
#include "util.h"


/** @file
* @brief Efficient range-based erase and copy functions
*
* Designed to interface with the standard library. Also contains non-member assign, append, insert functions.
*/

namespace oel
{

/** @brief Erase the element at index from container without maintaining order of elements after index.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template< typename RandomAccessContainer >
constexpr void erase_unstable(RandomAccessContainer & c, typename RandomAccessContainer::size_type index)
{
	c[index] = std::move(c.back());
	c.pop_back();
}

/**
* @brief Erase from container all elements for which predicate returns true
*
* This mimics `std::erase_if` (C++20) for sequence containers  */
template< typename Container, typename UnaryPredicate >
constexpr void erase_if(Container & c, UnaryPredicate p)   { _detail::RemoveIf(c, p, int{}); }
/**
* @brief Erase consecutive duplicate elements in container
*
* Calls Container::unique if available (with fallback std::unique).
* To erase duplicates anywhere, sort container contents first. (Or just use std::set or unordered_set)  */
template< typename Container >
constexpr void erase_adjacent_dup(Container & c)   { _detail::Unique(c, int{}); }



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
* Requires that  `source.size()` or `end(source) - begin(source)` is valid, and that dest models random_access_iterator.
* To move instead of copy, pass `view::move(source)`. To mimic std::copy_n, use view::counted.
* (Views can be used for all functions taking a range as source)  */
template< typename SizedInputRange, typename RandomAccessIter >  inline
auto copy_unsafe(SizedInputRange && source, RandomAccessIter dest)
->	copy_return<decltype(begin(source))>
	                                    { return {_detail::CopyUnsf(begin(source), _detail::Size(source), dest)}; }
/**
* @brief Copies the elements in source range into dest range, throws std::out_of_range if dest is smaller than source
* @return `begin(source)` incremented by the number of elements in source
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that `source.size()` or `end(source) - begin(source)` is valid, and that dest models random_access_range. */
template< typename SizedInputRange, typename RandomAccessRange >
auto copy(SizedInputRange && source, RandomAccessRange && dest)
->	copy_return<decltype(begin(source))>;
/**
* @brief Copies as many elements from source as will fit in dest
* @return true if all elements were copied, false means truncation happened
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that dest is a random_access_range (C++20 concept)  */
template< typename InputRange, typename RandomAccessRange >  inline
bool copy_fit(InputRange && source, RandomAccessRange && dest)   { return _detail::CopyFit(source, dest); }



/** @name GenericContainerInsert
* @brief For generic code that may use either dynarray or std library container (overloaded in dynarray.h)  */
//!@{
template< typename Container, typename InputRange >
constexpr void assign(Container & dest, InputRange && source)  { dest.assign(begin(source), end(source)); }

template< typename Container, typename InputRange >
constexpr void append(Container & dest, InputRange && source)  { dest.insert(dest.end(), begin(source), end(source)); }

template< typename Container, typename T >
constexpr void append(Container & dest, typename Container::size_type count, const T & val)
{
	dest.resize(dest.size() + count, val);
}

template< typename Container, typename ContainerIterator, typename InputRange >
constexpr auto insert_range(Container & dest, ContainerIterator pos, InputRange && source)
{
	return dest.insert(pos, begin(source), end(source));
}
//!@}
} // namespace oel



////////////////////////////////////////////////////////////////////////////////
//
// Only implementation


template< typename SizedInputRange, typename RandomAccessRange >
auto oel::copy(SizedInputRange && src, RandomAccessRange && dest)
->	copy_return<decltype(begin(src))>
{
	if (as_unsigned(_detail::Size(src)) <= as_unsigned(_detail::Size(dest)))
		return oel::copy_unsafe(src, begin(dest));
	else
		_detail::Throw::outOfRange("Too small dest oel::copy");
}