#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_range_util.h"

#include <algorithm>
#include <cstring>
#include <cstdint>

/**
* @file stl_util.h
* Utility functions designed for the STL interface
*/

namespace oetl
{

/// Given argument val of integral or enumeration type T, returns val cast to the signed integer type corresponding to T
template<typename T>
typename std::make_signed<T>::type    as_signed(T val) NOEXCEPT    { return typename std::make_signed<T>::type(val); }
/// Given argument val of integral or enumeration type T, returns val cast to the unsigned integer type corresponding to T
template<typename T>
typename std::make_unsigned<T>::type  as_unsigned(T val) NOEXCEPT  { return typename std::make_unsigned<T>::type(val); }



/// Check if index is valid (can be used with operator[]) for array or other range.
template<typename T, typename Range>
typename std::enable_if< std::is_unsigned<T>::value,
bool >::type  index_valid(const Range & r, T index);
/// Check if index is valid (can be used with operator[]) for array or other range.
template<typename Range>
bool index_valid(const Range & range, int32_t index);
/// Check if index is valid (can be used with operator[]) for array or other range.
template<typename Range>
bool index_valid(const Range & range, int64_t index);


/**
* @brief Erase the element at index from container without maintaining order of elements.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template<class RandomAccessContainer> inline
void erase_unordered(RandomAccessContainer & ctr, typename RandomAccessContainer::size_type index)
{
	ctr[index] = std::move(ctr.back());
	ctr.pop_back();
}
/**
* @brief Erase the element at position from container without maintaining order of elements.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template<class Container> inline
void erase_unordered(Container & ctr, typename Container::iterator position)
{
	*position = std::move(ctr.back());
	ctr.pop_back();
}

/// Erase the elements from newEnd to the end of container
template<class Container> inline
void truncate(Container & ctr, typename Container::iterator newEnd)  { ctr.erase(newEnd, ctr.end()); }

/**
* @brief Erase consecutive duplicate elements in container.
*
* To erase duplicates anywhere, sort container contents first. (Or just use std::set or unordered_set)  */
template<class Container>
void erase_successive_dup(Container & ctr)
{
	truncate( ctr, std::unique(ctr.begin(), ctr.end()) );
}


/**
* @brief Copies the elements in source into the range beginning at dest
* @return an iterator to the end of the destination range
*
* The ranges shall not overlap. To move instead of copy, include iterator_range.h and pass move_range(source)  */
template<typename InputRange, typename OutputIterator>
OutputIterator copy_nonoverlap(const InputRange & source, OutputIterator dest);
/**
* @brief Copies count elements from range beginning at first into the range beginning at dest
* @return a struct with iterators to the end of both ranges
*
* The ranges shall not overlap. To move instead of copy, pass a move_iterator as first  */
template<typename InputIterator, typename Count, typename OutputIterator>
range_ends<InputIterator, OutputIterator>  copy_nonoverlap(InputIterator first, Count count, OutputIterator dest);


////////////////////////////////////////////////////////////////////////////////

// Implementation only in rest of the file


namespace _detail
{
	template<typename InputIter, typename OutputIter> inline
	OutputIter Copy(std::false_type, InputIter first, InputIter last, OutputIter dest)
	{
		for (; first != last; ++dest, ++first)
			*dest = *first;

		return dest;
	}

	template<typename IterSrc, typename IterDest> inline
	IterDest Copy(std::true_type, IterSrc const first, IterSrc const last, IterDest const dest)
	{	// can use memcpy
		auto const count = last - first;
#	if OETL_MEM_BOUND_DEBUG_LVL
		if (0 != count)
		{	// Dereference iterators at bounds, this detects out of range errors if they are checked iterators
			*first; *dest;
			*(dest + (count - 1));
		}
#	endif
		memcpy(to_ptr(dest), to_ptr(first), count * sizeof(*first));
		return dest + count;
	}

	template<typename IterSrc, typename Count, typename IterDest> inline
	range_ends<IterSrc, IterDest> CopyN(std::true_type, IterSrc first, Count const count, IterDest dest)
	{	// can use memcpy
		if (0 < count)
		{
#		if OETL_MEM_BOUND_DEBUG_LVL
			*(first + (count - 1));        // Dereference iterators at bounds, this detects
			*dest; *(dest + (count - 1));  // out of range errors if they are checked iterators
#		endif
			memcpy(to_ptr(dest), to_ptr(first), count * sizeof(*first));
			first += count;
			dest += count;
		}
		return {first, dest};
	}

	template<typename InputIter, typename Count, typename OutputIter> inline
	range_ends<InputIter, OutputIter> CopyN(std::false_type, InputIter first, Count count, OutputIter dest)
	{
		for (; 0 < count; --count)
		{
			*dest = *first;
			++dest; ++first;
		}
		return {first, dest};
	}
}

} // namespace oetl

template<typename InputRange, typename OutputIterator>
inline OutputIterator oetl::copy_nonoverlap(const InputRange & source, OutputIterator dest)
{
	return _detail::Copy(can_memmove_ranges_with(dest, begin(source)),
						 begin(source), end(source), dest);
}

template<typename InputIterator, typename Count, typename OutputIterator>
inline oetl::range_ends<InputIterator, OutputIterator>  oetl::
	copy_nonoverlap(InputIterator first, Count count, OutputIterator dest)
{
	return _detail::CopyN(can_memmove_ranges_with(dest, first),
						  first, count, dest);
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename Range>
inline typename std::enable_if< std::is_unsigned<T>::value,
	bool >::type  oetl::index_valid(const Range & r, T idx)
{
	return idx < as_unsigned(count(r));
}

template<typename Range>
inline bool oetl::index_valid(const Range & r, int32_t idx)
{
	return 0 <= idx && idx < count(r);
}

template<typename Range>
inline bool oetl::index_valid(const Range & r, int64_t idx)
{
	return static_cast<uint64_t>(idx) < static_cast<uint64_t>(count(r));
}
