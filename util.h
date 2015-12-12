#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

#include <algorithm>
#include <memory>
#include <cstdint>
#include <string.h>


/** @file
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
void erase_unordered(RandomAccessContainer & c, typename RandomAccessContainer::size_type index)
{
	c[index] = std::move(c.back());
	c.pop_back();
}
/**
* @brief Erase the element at pos from container without maintaining order of elements.
* @param pos dereferenceable iterator (not the end), can simply be a pointer to an element in c
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template<typename OutputIterator, typename Container,
		 typename /*SFINAE*/ = decltype( *std::declval<OutputIterator>() )> inline
void erase_unordered(Container & c, OutputIterator pos)
{
	*pos = std::move(c.back());
	c.pop_back();
}

/// Erase the elements from first to the end of container, useful for std::remove_if and similar
template<typename Container> inline
void erase_back(Container & c, typename Container::iterator first)  { c.erase(first, c.end()); }


/**
* @brief Copies the elements in source into the range beginning at dest
* @return an iterator to the end of the destination range
*
* The ranges shall not overlap, except if begin(source) equals dest (self assign).
* To move instead of copy, include iterator_range.h and pass move_range(source)  */
template<typename InputRange, typename OutputIterator>
OutputIterator copy(const InputRange & source, OutputIterator dest);



/// Check if index is valid (can be used with operator[]) for array or other range.
template< typename UnsignedInt, typename SizedRange,
		  typename = enable_if_t<std::is_unsigned<UnsignedInt>::value> > inline
bool index_valid(const SizedRange & r, UnsignedInt index)   { return index < oel::as_unsigned(oel::ssize(r)); }
/// Check if index is valid (can be used with operator[]) for array or other range.
template<typename SizedRange> inline
bool index_valid(const SizedRange & r, std::int32_t index)  { return 0 <= index && index < oel::ssize(r); }
/// Check if index is valid (can be used with operator[]) for array or other range.
template<typename SizedRange> inline
bool index_valid(const SizedRange & r, std::int64_t index)
{	// assumes that r.size() is never greater than INT64_MAX
	return static_cast<std::uint64_t>(index) < oel::as_unsigned(oel::ssize(r));
}


/**
* @brief Returns new T(std::forward<Args>(args)...) if T is constructible from Args, else new T{std::forward<Args>(args)...}
*
* Helper as suggested in section "Is that all?" http://open-std.org/JTC1/SC22/WG21/docs/papers/2015/n4462.html  */
template<typename T, typename... Args>
T * forward_to_new(Args &&... args);


/// Equivalent to std::make_unique. Performs direct-list-initialization if there is no matching constructor
template< typename T, typename... Args, typename = enable_if_t<!std::is_array<T>::value> > inline
std::unique_ptr<T> make_unique(Args &&... args)
{
	T * p = oel::forward_to_new<T>(std::forward<Args>(args)...);
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
	template<typename InputIter, typename Sentinel, typename OutputIter> inline
	OutputIter Copy(std::false_type, InputIter first, Sentinel last, OutputIter dest)
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
}
/// @endcond

} // namespace oel

template<typename InputRange, typename OutputIterator>
inline OutputIterator oel::copy(const InputRange & src, OutputIterator dest)
{
	return _detail::Copy(can_memmove_with<OutputIterator, decltype(begin(src))>(),
						 begin(src), end(src), dest);
}

////////////////////////////////////////////////////////////////////////////////

namespace oel
{
namespace _detail
{
	template<typename T, typename... Args>
	T * New(std::true_type, Args &&... args)
	{
		return new T(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	T * New(std::false_type, Args &&... args)
	{
		return new T{std::forward<Args>(args)...};
	}
}
}

template<typename T, typename... Args>
T * oel::forward_to_new(Args &&... args)
{
	return _detail::New<T>(std::is_constructible<T, Args...>(), std::forward<Args>(args)...);
}


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
