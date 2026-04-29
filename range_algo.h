#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "dynarray.h"
#include "util.h"  // for as_unsigned
#include "auxi/range_algo_detail.h"
#include "view/counted.h"


/** @file
* @brief Efficient range-based erase, copy functions, concat_to_dynarray and non-member append
*
* Designed to interface with the standard library.
*/

namespace oel
{

//! Equivalent to oel::concat_to_dynarray with an allocator instance for the dynarray
/** @param a will be rebound to the type deduced from Ranges, which is std::common_type_t of all range value types
*
* Has the same effect as `std::views::concat(sources...) | to_dynarray(a)`. */
template< typename Alloc, typename... Ranges >
auto concat_to_dynarray_with_alloc(Alloc a, Ranges &&... sources)
	{
		static_assert((... and _detail::rangeIsForwardOrSized<Ranges>));
		using T = std::common_type_t<
				iter_value_t< iterator_t<Ranges> >...
			>;
		size_t const counts[]{_detail::UDist(sources)...};

		size_t sum{};
		for (auto n : counts)
			sum += n;

		auto d = dynarray<T, Alloc>(reserve, sum, std::move(a));

		auto nIt = begin(counts);
		(..., d.append( view::counted(begin(sources), *nIt++) ));

		return d;
	}

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
		return oel::concat_to_dynarray_with_alloc(allocator<>{}, sources...);
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



template< typename Iterator >
struct copy_return
{
	Iterator in;
};

//! Copies the elements in source into the range beginning at dest
/** @return `begin(source)` incremented by source size
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that `source.size()` or `end(source) - begin(source)` is valid, and that dest models random_access_iterator.
* To move instead of copy, wrap source with view::move. (To mimic std::copy_n, use view::counted)  */
template< typename SizedRange, typename RandomAccessIter >  inline
auto copy_unsafe(SizedRange && source, RandomAccessIter dest)
->	copy_return< borrowed_iterator_t<SizedRange> >
	{
		return{ _detail::Copy(begin(source), _detail::Size(source), dest) };
	}

//! Copies as many elements from source range as will fit in dest range
/** @return `begin(source)` incremented by source size
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that dest models std::ranges::random_access_range. */
template< typename InputRange, typename RandomAccessRange >
auto copy_fit(InputRange && source, RandomAccessRange && dest)
->	copy_return< borrowed_iterator_t<InputRange> >;



//! Generic way to call append_range or append on a container or string, with a source range
inline constexpr auto append =
	[](auto & container, auto && source)
	{
		using Ref = decltype(source);
	#if __cpp_concepts >= 201907
		if constexpr (requires{ container.append_range(static_cast<Ref>(source)); })
			container.append_range(static_cast<Ref>(source));
		else
	#endif
		if constexpr (decltype( _detail::CanAppend(container, static_cast<Ref>(source)) )::value)
			container.append(static_cast<Ref>(source));
		else
			container.append(to_pointer_contiguous(begin(source)), _detail::Size(source));
	};

} // namespace oel



////////////////////////////////////////////////////////////////////////////////
//
// Just implementation


template< typename InputRange, typename RandomAccessRange >
auto oel::copy_fit(InputRange && source, RandomAccessRange && dest)
->	copy_return< borrowed_iterator_t<InputRange> >
{
	if constexpr (range_is_sized<InputRange>)
	{
		auto       n        = as_unsigned(_detail::Size(source));
		auto const destSize = as_unsigned(_detail::Size(dest));
		if (n > destSize)
			n = destSize;

		return {_detail::Copy( begin(source), n, begin(dest) )};
	}
	else
	{	auto it = begin(source); auto const last = end(source);
		auto di = begin(dest);   auto const dl = end(dest);
		while (it != last and di != dl)
		{
			*di = *it;
			++di; ++it;
		}
		return {it};
	}
}