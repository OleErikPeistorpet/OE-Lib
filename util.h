#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

#include <algorithm>
#include <cstdint>
#include <string.h>

/**
* @file util.h
* @brief Utilities, including algorithms
*
* Designed to interface with the standard library. Contains erase functions, copy functions and more.
*/

namespace oel
{

/// Given argument val of integral or enumeration type T, returns val cast to the signed integer type corresponding to T
template<typename T> inline
typename std::make_signed<T>::type    as_signed(T val) NOEXCEPT    { return typename std::make_signed<T>::type(val); }
/// Given argument val of integral or enumeration type T, returns val cast to the unsigned integer type corresponding to T
template<typename T> inline
typename std::make_unsigned<T>::type  as_unsigned(T val) NOEXCEPT  { return typename std::make_unsigned<T>::type(val); }


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

/// Erase the elements from first to the end of container, making first the new end
template<class Container> inline
void erase_back(Container & ctr, typename Container::iterator first)  { ctr.erase(first, ctr.end()); }

/**
* @brief Erase consecutive duplicate elements in container.
*
* To erase duplicates anywhere, sort container contents first. (Or just use std::set or unordered_set)  */
template<class Container>
void erase_successive_dup(Container & ctr)
{
	erase_back( ctr, std::unique(begin(ctr), end(ctr)) );
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



template<bool Condition>
using enable_if_t = typename std::enable_if<Condition>::type;


/// Check if index is valid (can be used with operator[]) for array or other range.
template< typename T, typename Range, typename = enable_if_t<std::is_unsigned<T>::value> >
bool index_valid(const Range & range, T index);
/// Check if index is valid (can be used with operator[]) for array or other range.
template<typename Range>
bool index_valid(const Range & range, std::int32_t index);
/// Check if index is valid (can be used with operator[]) for array or other range.
template<typename Range>
bool index_valid(const Range & range, std::int64_t index);


////////////////////////////////////////////////////////////////////////////////

// Implementation only in rest of the file


/// @cond INTERNAL
namespace _detail
{
	template<typename InputIter, typename OutputIter> inline
	OutputIter Copy(std::false_type, InputIter first, InputIter last, OutputIter dest)
	{
		while (first != last)
		{
			*dest = *first;
			++dest; ++first;
		}
		return dest;
	}

	template<typename IterSrc, typename IterDest> inline
	IterDest Copy(std::true_type, IterSrc const first, IterSrc const last, IterDest const dest)
	{	// can use memcpy
		auto const count = last - first;
#	if OEL_MEM_BOUND_DEBUG_LVL
		if (0 != count)
		{	// Dereference iterators at bounds, this detects out of range errors if they are checked iterators
		OEL_PUSH_IGNORE_UNUSED_VALUE
			*first; *dest;
			*(dest + (count - 1));
		OEL_POP_DIAGNOSTIC
		}
#	endif
		::memcpy(to_ptr(dest), to_ptr(first), count * sizeof(*first));
		return dest + count;
	}

	template<typename IterSrc, typename Count, typename IterDest> inline
	range_ends<IterSrc, IterDest> CopyN(std::true_type, IterSrc first, Count const count, IterDest dest)
	{	// can use memcpy
		if (0 < count)
		{
#		if OEL_MEM_BOUND_DEBUG_LVL
			OEL_PUSH_IGNORE_UNUSED_VALUE
			*(first + (count - 1));        // Dereference iterators at bounds, this detects
			*dest; *(dest + (count - 1));  // out of range errors if they are checked iterators
			OEL_POP_DIAGNOSTIC
#		endif
			::memcpy(to_ptr(dest), to_ptr(first), count * sizeof(*first));
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
/// @endcond

} // namespace oel

template<typename InputRange, typename OutputIterator>
inline OutputIterator oel::copy_nonoverlap(const InputRange & source, OutputIterator dest)
{
	return _detail::Copy(can_memmove_with<OutputIterator, decltype(begin(source))>(),
						 begin(source), end(source), dest);
}

template<typename InputIterator, typename Count, typename OutputIterator>
inline oel::range_ends<InputIterator, OutputIterator>  oel::
	copy_nonoverlap(InputIterator first, Count count, OutputIterator dest)
{
	return _detail::CopyN(can_memmove_with<OutputIterator, InputIterator>(),
						  first, count, dest);
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename Range, typename>
inline bool oel::index_valid(const Range & r, T idx)
{
	return idx < as_unsigned(oel::count(r));
}

template<typename Range>
inline bool oel::index_valid(const Range & r, std::int32_t idx)
{
	return 0 <= idx && idx < oel::count(r);
}

template<typename Range>
inline bool oel::index_valid(const Range & r, std::int64_t idx)
{
	auto idxU = static_cast<std::uint64_t>(idx);
	return idxU < as_unsigned(oel::count(r)); // assumes that r size is never greater than INT64_MAX
}