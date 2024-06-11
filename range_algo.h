#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/range_algo_detail.h"
#include "util.h"


/** @file
* @brief Efficient range-based erase, copy functions and non-member append
*
* Designed to interface with the standard library.
*/

namespace oel
{

/** @brief Erase the element at index from container without maintaining order of elements after index.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template< typename Integer, typename RandomAccessContainer >
constexpr void unordered_erase(RandomAccessContainer & c, Integer index)
	{
		if constexpr (decltype( _detail::HasUnorderedErase(c) )::value)
		{
			c.unordered_erase(c.begin() + index);
		}
		else
		{	c[index] = std::move(c.back());
			c.pop_back();
		}
	}

/** @brief Erase from container all elements for which predicate returns true
*
* This mimics `std::erase_if` (C++20) for sequence containers  */
template< typename Container, typename UnaryPredicate >
constexpr void erase_if(Container & c, UnaryPredicate p)   { _detail::RemoveIf(c, std::move(p)); }
/**
* @brief Erase consecutive duplicate elements in container
*
* Calls Container::unique if available (with fallback std::unique).
* To erase duplicates anywhere, sort container contents first. (Or just use std::set or unordered_set)  */
template< typename Container >
constexpr void erase_adjacent_dup(Container & c)   { _detail::Unique(c); }



template< typename Iterator >
struct copy_return
{
	Iterator in;
};
/**
* @brief Copies the elements in source range into the range beginning at iter
* @return `begin(source)` incremented by source size
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that  `source.size()` or `end(source) - begin(source)` is valid, and that iter models random_access_iterator.
* To move instead of copy, wrap source with view::move. To mimic std::copy_n, use view::counted.
* (Views can be used for all functions taking a range as source)  */
inline constexpr auto copy_unsafe =
	[](auto && source, auto iter) -> copy_return< borrowed_iterator_t<decltype(source)> >
	{
		return{ _detail::CopyUnsf(begin(source), _detail::Size(source), iter) };
	};
/**
* @brief Copies the elements in source range into dest range, throws std::out_of_range if dest is smaller than source
* @return `begin(source)` incremented by the number of elements in source
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that `source.size()` or `end(source) - begin(source)` is valid, and that dest models random_access_range. */
inline constexpr auto copy =
	[](auto && source, auto && dest) -> copy_return< borrowed_iterator_t<decltype(source)> >
	{
		if (as_unsigned( _detail::Size(source) ) <= as_unsigned( _detail::Size(dest) ))
			return copy_unsafe(source, begin(dest));
		else
			_detail::OutOfRange::raise("Too small dest oel::copy");
	};
/**
* @brief Copies as many elements from source range as will fit in dest range
* @return true if all elements were copied, false means truncation happened
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that dest models std::ranges::random_access_range. */
inline constexpr auto copy_fit =
	[](auto && source, auto && dest) -> bool   { return _detail::CopyFit(source, dest); };



struct _appendFn
{
	template< typename Container, typename InputRange >
	void operator()(Container & c, InputRange && source) const
	{
	#if __cpp_concepts >= 201907
		if constexpr (requires{ c.append_range(static_cast<InputRange &&>(source)); })
			c.append_range(static_cast<InputRange &&>(source));
		else
	#endif
			c.insert(c.end(), begin(source), end(source));
	}

	template< typename T, typename A, typename InputRange >
	void operator()(dynarray<T, A> & c, InputRange && source) const
	{
		c.append(static_cast<InputRange &&>(source));
	}

	template< typename T, size_t C, typename S, typename InputRange >
	void operator()(inplace_growarr<T, C, S> & c, InputRange && source) const
	{
		c.append(static_cast<InputRange &&>(source));
	}
};
/** @brief Append source range at end of a container
*
* Generic function for use with dynarray or container that has standard library interface. */
inline constexpr _appendFn append;

} // namespace oel
