#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

#include <algorithm>
#include <cstdint>
#include <string.h>

/**
* @file util.h
* @brief Utilities, including efficient range algorithms
*
* Designed to interface with the standard library. Contains erase functions, copy functions, make_unique and more.
*/

namespace oel
{

/// Given argument val of integral or enumeration type T, returns val cast to the signed integer type corresponding to T
template<typename T> inline
constexpr make_signed_t<T>   as_signed(T val) noexcept    { return (make_signed_t<T>)val; }
/// Given argument val of integral or enumeration type T, returns val cast to the unsigned integer type corresponding to T
template<typename T> inline
constexpr make_unsigned_t<T> as_unsigned(T val) noexcept  { return (make_unsigned_t<T>)val; }



/** @brief Erase the element at index from container without maintaining order of elements.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template<typename RandomAccessContainer> inline
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
template<typename OutputIterator, typename Container,
		 typename = decltype( *std::declval<OutputIterator>() )> inline
void erase_unordered(Container & ctr, OutputIterator position)
{
	*position = std::move(ctr.back());
	ctr.pop_back();
}

/// Erase the elements from first to the end of container, useful for std::remove_if and similar
template<typename Container> inline
void erase_back(Container & ctr, typename Container::iterator first)  { ctr.erase(first, ctr.end()); }

/**
* @brief Erase consecutive duplicate elements in container.
*
* To erase duplicates anywhere, sort container contents first. (Or just use std::set or unordered_set)  */
template<typename Container>
void erase_successive_dup(Container & ctr);


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
* The ranges shall not overlap. To move instead of copy, pass a std::move_iterator as first  */
template<typename InputIterator, typename Count, typename OutputIterator>
range_ends<InputIterator, OutputIterator>  copy_nonoverlap(InputIterator first, Count count, OutputIterator dest);



/// Exists in std with C++14
template<bool Condition>
using enable_if_t = typename std::enable_if<Condition>::type;


/// Check if index is valid (can be used with operator[]) for array or other range.
template< typename UnsignedInt, typename Range,
		  typename = enable_if_t<std::is_unsigned<UnsignedInt>::value> > inline
bool index_valid(const Range & r, UnsignedInt index)   { return index < oel::as_unsigned(oel::ssize(r)); }
/// Check if index is valid (can be used with operator[]) for array or other range.
template<typename Range> inline
bool index_valid(const Range & r, std::int32_t index)  { return 0 <= index && index < oel::ssize(r); }
/// Check if index is valid (can be used with operator[]) for array or other range.
template<typename Range> inline
bool index_valid(const Range & r, std::int64_t index)
{	// assumes that r.size() is never greater than INT64_MAX
	return static_cast<std::uint64_t>(index) < oel::as_unsigned(oel::ssize(r));
}



/// Equivalent to std::make_unique.
template< typename T, typename... Args, typename = enable_if_t<!std::is_array<T>::value> > inline
std::unique_ptr<T> make_unique(Args &&... args)
{
	T * p = new T(std::forward<Args>(args)...); // direct-initialize, or value-initialize if no args
	return std::unique_ptr<T>(p);
}
/// Equivalent to std::make_unique (array version).
template< typename T, typename = enable_if_t<std::is_array<T>::value> >
std::unique_ptr<T> make_unique(size_t arraySize);
/**
* @brief Array is default-initialized, can be significantly faster for non-class elements
*
* Non-class elements get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
template< typename T, typename = enable_if_t<std::is_array<T>::value> >
std::unique_ptr<T> make_unique_default(size_t arraySize);



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


/// @cond INTERNAL
namespace _detail
{
	template<typename HasUnique> inline // pass dummy int to prefer this overload
	auto Unique(HasUnique & ctr, int) -> decltype(ctr.unique()) { return ctr.unique(); }

	template<typename Container>
	void Unique(Container & ctr, long)
	{
		erase_back( ctr, std::unique(begin(ctr), end(ctr)) );
	}

////////////////////////////////////////////////////////////////////////////////

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
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (0 != count)
		{	// Dereference iterators at bounds, this detects out of range errors if they are checked iterators
			(void)*first; (void)*dest;
			(void)*(dest + (count - 1));
		}
	#endif
		::memcpy(to_pointer_contiguous(dest), to_pointer_contiguous(first), sizeof(*first) * count);
		return dest + count;
	}

	template<typename IterSrc, typename Count, typename IterDest> inline
	range_ends<IterSrc, IterDest> CopyN(std::true_type, IterSrc first, Count const count, IterDest dest)
	{	// can use memcpy
		if (0 < count)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			(void)*(first + (count - 1)); // Dereference iterators at bounds, this detects
			(void)*dest;                  // out of range errors if they are checked iterators
			(void)*(dest + (count - 1));
		#endif
			::memcpy(to_pointer_contiguous(dest), to_pointer_contiguous(first), sizeof(*first) * count);
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


template<typename Container>
inline void oel::erase_successive_dup(Container & ctr)
{
	_detail::Unique(ctr, int{});
}

////////////////////////////////////////////////////////////////////////////////

#define OEL_MAKE_UNIQUE(newExpr)  \
	static_assert(std::extent<T>::value == 0, "make_unique forbids T[size]. Please use T[]");  \
	using Elem = typename std::remove_extent<T>::type;  \
	return std::unique_ptr<T>(newExpr)

template<typename T, typename>
inline std::unique_ptr<T>  oel::make_unique(size_t size)
{
	OEL_MAKE_UNIQUE( new Elem[size]() ); // value-initialize
}

template<typename T, typename>
inline std::unique_ptr<T>  oel::make_unique_default(size_t size)
{
	OEL_MAKE_UNIQUE(new Elem[size]);
}

#undef OEL_MAKE_UNIQUE
