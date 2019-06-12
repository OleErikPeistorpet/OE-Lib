#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/algo_detail.h"
#include "range_view.h"

#include <algorithm>


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
void erase_unstable(RandomAccessContainer & c, typename RandomAccessContainer::size_type index)
{
	c[index] = std::move(c.back());
	c.pop_back();
}

/**
* @brief Erase from container all elements for which predicate returns true
*
* This mimics `std::erase_if` (C++20) for sequence containers  */
template< typename Container, typename UnaryPredicate >
void erase_if(Container & c, UnaryPredicate p);
/**
* @brief Erase consecutive duplicate elements in container
*
* Calls Container::unique if available (with fallback std::unique).
* To erase duplicates anywhere, sort container contents first. (Or just use std::set or unordered_set)  */
template< typename Container >
void erase_adjacent_dup(Container & c);



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
template< typename SizedInputRange, typename RandomAccessIter >
auto copy_unsafe(const SizedInputRange & source, RandomAccessIter dest)
->	copy_return<decltype(begin(source))>;
/**
* @brief Copies the elements in source range into dest range, throws std::out_of_range if dest is smaller than source
* @return `begin(source)` incremented by the number of elements in source
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that source has size() member or is an array, and dest is a random_access_range (C++20 concept)  */
template< typename SizedInputRange, typename RandomAccessRange >
auto copy(const SizedInputRange & source, RandomAccessRange && dest)
->	copy_return<decltype(begin(source))>;
/**
* @brief Copies as many elements from source as will fit in dest
* @return true if all elements were copied, false means truncation happened
* @pre If the ranges overlap, behavior is undefined (uses memcpy when possible)
*
* Requires that dest is a random_access_range (otherwise compilation will fail)  */
template< typename InputRange, typename RandomAccessRange >
bool copy_fit(const InputRange & source, RandomAccessRange && dest);



/** @name GenericContainerInsert
* @brief For generic code that may use either dynarray or std library container (overloaded in dynarray.h)  */
//!@{
template< typename Container, typename InputRange >  inline
void assign(Container & dest, const InputRange & source)  { dest.assign(begin(source), end(source)); }

template< typename Container, typename InputRange >  inline
void append(Container & dest, const InputRange & source)  { dest.insert(dest.end(), begin(source), end(source)); }

template< typename Container, typename T >  inline
void append(Container & dest, typename Container::size_type count, const T & val)
{
	dest.resize(dest.size() + count, val);
}

template< typename Container, typename ContainerIterator, typename InputRange >  inline
typename Container::iterator insert(Container & dest, ContainerIterator pos, const InputRange & source)
{
	return dest.insert(pos, begin(source), end(source));
}
//!@}



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


namespace _detail
{
	template< typename Container >
	inline auto EraseEnd(Container & c, typename Container::iterator f)
	->	decltype(c.erase_to_end(f))
		{ return c.erase_to_end(f); }

	template< typename Container, typename ContainerIter, typename... None >
	inline void EraseEnd(Container & c, ContainerIter f, None...) { c.erase(f, c.end()); }


	template< typename Container, typename UnaryPred >
	inline auto RemoveIf(Container & c, UnaryPred p, int)
	->	decltype(c.remove_if(p)) { return c.remove_if(p); }

	template< typename Container, typename UnaryPred >
	void RemoveIf(Container & c, UnaryPred p, long)
	{
		_detail::EraseEnd( c, std::remove_if(begin(c), end(c), p) );
	}

	template< typename Container >
	inline auto Unique(Container & c, int)  // pass dummy int to prefer this overload
	->	decltype(c.unique()) { return c.unique(); }

	template< typename Container >
	void Unique(Container & c, long)
	{
		_detail::EraseEnd( c, std::unique(begin(c), end(c)) );
	}

////////////////////////////////////////////////////////////////////////////////

	template< typename ContiguousIter, typename ContiguousIter2,
	          enable_if< can_memmove_with<ContiguousIter2, ContiguousIter>::value > = 0
	>
	ContiguousIter CopyUnsf(ContiguousIter const src, ptrdiff_t const n, ContiguousIter2 const dest)
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (0 != n)
		{	// Dereference to detect out of range errors if the iterator has internal check
			(void) *dest;
			(void) *(dest + (n - 1));
		}
	#endif
		_detail::MemcpyCheck(src, n, to_pointer_contiguous(dest));
		return src + n;
	}

	template< typename InputIter, typename Integral, typename RandomAccessIter, typename... None >
	InputIter CopyUnsf(InputIter src, Integral const n, RandomAccessIter const dest, None...)
	{
		for (Integral i = 0; i < n; ++i)
		{
			dest[i] = *src;
			++src;
		}
		return src;
	}


	template< typename InputRange, typename OutputRange, typename... None >
	bool CopyFit(const InputRange & src, OutputRange & dest, None...)
	{
		auto it = begin(src);  auto const last = end(src);
		auto di = begin(dest);  auto const dl = end(dest);
		while (it != last)
		{
			if (di != dl)
			{
				*di = *it;
				++di; ++it;
			}
			else
			{	return false;
			}
		}
		return true;
	}

	template< typename SizedRange, typename RandomAccessRange >
	auto CopyFit(const SizedRange & src, RandomAccessRange & dest)
	->	decltype( oel::ssize(src), bool() ) // better match if ssize(src) is well-formed (SFINAE)
	{
		auto const destSize = oel::ssize(dest);
		auto n = oel::ssize(src);
		bool const success{n <= destSize};
		if (!success)
			n = destSize;

		oel::copy_unsafe(view::counted(begin(src), n), begin(dest));
		return success;
	}
}

} // namespace oel

template< typename SizedInputRange, typename RandomAccessIter >
inline auto oel::copy_unsafe(const SizedInputRange & src, RandomAccessIter dest)
->	copy_return<decltype(begin(src))>
{
	return {_detail::CopyUnsf(begin(src), oel::ssize(src), dest)};
}

template< typename SizedInputRange, typename RandomAccessRange >
auto oel::copy(const SizedInputRange & src, RandomAccessRange && dest)
->	copy_return<decltype(begin(src))>
{
	if (oel::ssize(src) <= oel::ssize(dest))
		return oel::copy_unsafe(src, begin(dest));
	else
		_detail::Throw::OutOfRange("Too small dest oel::copy");
}

template< typename InputRange, typename RandomAccessRange >
inline bool oel::copy_fit(const InputRange & src, RandomAccessRange && dest)
{
	return _detail::CopyFit(src, dest);
}


template< typename Container, typename UnaryPredicate >
inline void oel::erase_if(Container & c, UnaryPredicate p)
{
	_detail::RemoveIf(c, p, int{});
}

template< typename Container >
inline void oel::erase_adjacent_dup(Container & c)
{
	_detail::Unique(c, int{});
}