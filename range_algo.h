#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/range_algo_detail.h"
#include "util.h"


/** @file
* @brief Efficient erase, concat_to_dynarray and non-member append
*
* Designed to interface with the standard library.
*/

namespace oel
{

//! Concatenate multiple ranges into a dynarray using a single memory allocation
/**
* Requires that each of Ranges model std::ranges::forward_range or is a sized range.
* Example:
@code
constexpr auto header = "v1\n"sv;
std::string_view body();

auto result = concat_to_dynarray(header, body());
@endcode  */
template< typename... Ranges >  inline
auto concat_to_dynarray(Ranges &&... sources)
	{
		return _detail::ConcatToDynarr(allocator<>{}, static_cast<Ranges &&>(sources)...);
	}
//! Equivalent to oel::concat_to_dynarray with an allocator instance for the dynarray
/** @param a will be rebound to the type deduced from Ranges, which is std::common_type_t of all range value types
*
* Has the same effect as `std::views::concat(sources...) | to_dynarray(a)`. */
template< typename Alloc, typename... Ranges >  inline
auto concat_to_dynarray_with_alloc(Alloc a, Ranges &&... sources)
	{
		return _detail::ConcatToDynarr(std::move(a), static_cast<Ranges &&>(sources)...);
	}


/** @brief Erase the element at index from container without maintaining order of elements after index.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template< typename Integer, typename RandomAccessContainer >
constexpr void unordered_erase(RandomAccessContainer & c, Integer index)
{
	c[index] = std::move(c.back());
	c.pop_back();
}
//! See unordered_erase(RandomAccessContainer &, Integer) or dynarray::unordered_erase
template< typename Integer, typename T, typename A >  inline
void unordered_erase(dynarray<T, A> & d, Integer index)  { d.unordered_erase(d.begin() + index); }

/**
* @brief Erase from container all elements for which predicate returns true
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
		if constexpr (decltype( _detail::CanAppend(c, static_cast<InputRange &&>(source)) )::value)
			c.append(static_cast<InputRange &&>(source));
		else
			c.append(to_pointer_contiguous(begin(source)), _detail::Size(source));
	}
};
//! Generic way to call append_range or append on a container or string, with a source range
inline constexpr _appendFn append;

} // namespace oel
