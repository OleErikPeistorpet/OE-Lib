#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#include <algorithm>
#include <memory>
#include <limits>
#include <array>
#include <string.h>
#include <memory>

/**
* @file util.h
* @brief Utilities, including algorithms
*
* Designed to interface with the standard library. Contains erase functions, copy functions, make_unique and more.
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

/// Same as std::move(InIterator, InIterator, OutIterator), but much faster for arrays of simple classes than VC++ version
template<typename InputIterator, typename OutputIterator>
OutputIterator move(InputIterator first, InputIterator last, OutputIterator dest);

/// Throws exception if dest is smaller than source
template<typename SizedInRange, typename SizedOutRange>
auto copy(const SizedInRange & src, SizedOutRange & dest) -> decltype(begin(dest));
/// Copies as many elements as will fit in dest
template<typename RandomAccessRange, typename SizedOutRange>
auto copy_fit(const RandomAccessRange & src, SizedOutRange & dest) -> decltype(begin(dest));


/// Check if index is valid (can be used with operator[]) for array or other iterable.
template<typename Integer, typename CountableRange>
bool index_valid(const CountableRange & ci, Integer index);


///
template<size_t I, typename T, size_t N>
T &       get(T(&arr)[N]);
template<size_t I, typename T, size_t N>
const T & get(const T(&arr)[N]);



/// Equivalent to std::make_unique.
template< typename T, typename... ArgTs, typename = enable_if_t<!std::is_array<T>::value> > inline
std::unique_ptr<T> make_unique(ArgTs &&... args)
{
	T * p = new T(std::forward<ArgTs>(args)...); // direct-initialize, or value-initialize if no args
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

/// Calls new T with args as the parameter list and stores it in std::unique_ptr.
template<typename T, typename... ArgTs, typename = std::enable_if_t<!std::is_array<T>::value> >
void set_new(std::unique_ptr<T> & ptr, ArgTs &&... args);
/// Calls new T[arraySize]() and stores it in std::unique_ptr.
template<typename T>
void set_new(std::unique_ptr<T[]> & ptr, size_t arraySize);



/// Helper for make_array
template<typename T, typename...>
struct deduce_array_type { using type = T; };
/// Helper for make_array
template<typename... Ts>
struct deduce_array_type<void, Ts...>
{
	using type = typename std::decay< typename std::common_type<Ts...>::type >::type;
};

/// Returns std::array of same size as the number or arguments, with type deduced unless specified
template<typename T = void, typename... Args>
std::array<typename deduce_array_type<T, Args...>::type, sizeof...(Args)>
	make_array(Args &&... args)  { return { std::forward<Args>(args)... }; }

/// Returns std::array of same size as the number or arguments, all arguments static_cast to T
template<typename T, typename... Args>
std::array<T, sizeof...(Args)>  make_array_static_cast(Args &&... args);



template<typename T>
struct identity { using type = T; };

template<typename T>
using identity_t = typename identity<T>::type;

/**
* @brief Bring the value into the range [low, high].
*
* @return If val is greater than high, return high. If val is less than low, return low. Otherwise, return val. */
template<typename T>
constexpr T clamp(const T & val, const identity_t<T> & low, const identity_t<T> & high)
{
	//OEL_ASSERT( !(high < low) );
	return high < val ?
		high :
		(val < low ? low : val);
}



///
template<typename Count, typename T, typename InputIterator>
Count find_idx(InputIterator first, Count count, const T & value);
///
template<typename T, typename InputRange> inline
auto find_idx(const InputRange & toSearch, const T & value) -> difference_type<decltype(begin(toSearch))>
{
	return oel::find_idx(begin(toSearch), oel::count(toSearch), value);
}

///
template<typename T, typename BidirectionRange> inline
auto rfind_idx(const BidirectionRange & toSearch, const T & value) -> difference_type<decltype(begin(toSearch))>
{
	auto pos = oel::count(toSearch);
	auto it = end(toSearch);
	while (--pos != -1)
	{
		--it;
		if (*it == value)
			break;
	}
	return pos;
}


///
template<typename ForwardRange, typename T>
auto find_sorted(ForwardRange & ib, const T & val) -> decltype(begin(ib));
///
template<typename ForwardRange, typename T, typename Compare>
auto find_sorted(ForwardRange & ib, const T & val, Compare comp) -> decltype(begin(ib));



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


/// @cond INTERNAL
namespace _detail
{
	template<typename InputIter, typename Sentinel, typename OutputIter, typename Unused> inline
	OutputIter Copy(std::false_type, Unused, InputIter first, Sentinel last, OutputIter dest)
	{
		while (first != last)
		{
			*dest = *first;
			++dest; ++first;
		}
		return dest;
	}

	template<typename IterSrc, typename IterDest, typename TrivialCopyFunc> inline
	IterDest Copy(std::true_type, TrivialCopyFunc doCopy,
				  IterSrc const first, IterSrc const last, IterDest const dest)
	{	// can use memcpy/memmove
		auto const count = last - first;
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (0 != count)
		{	// Dereference iterators at bounds, this detects out of range errors if they are checked iterators
		OEL_PUSH_IGNORE_UNUSED_VALUE
			*first; *dest;
			*(dest + (count - 1));
		OEL_POP_DIAGNOSTIC
		}
	#endif
		doCopy(to_pointer_contiguous(dest), to_pointer_contiguous(first), sizeof(*first) * count);
		return dest + count;
	}

	template<typename IterSrc, typename Count, typename IterDest> inline
	range_ends<IterSrc, IterDest> CopyN(std::true_type, IterSrc first, Count const count, IterDest dest)
	{	// can use memcpy
		if (0 < count)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			OEL_PUSH_IGNORE_UNUSED_VALUE
			*(first + (count - 1));        // Dereference iterators at bounds, this detects
			*dest; *(dest + (count - 1));  // out of range errors if they are checked iterators
			OEL_POP_DIAGNOSTIC
		#endif
			::memcpy(to_pointer_contiguous(dest), to_pointer_contiguous(first), count * sizeof(*first));
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
inline OutputIterator oel::copy_unsafe(const InputRange & src, OutputIterator dest)
{
	return _detail::Copy(can_memmove_with<OutputIterator, decltype(begin(src))>(),
						 ::memcpy,
						 begin(src), end(src), dest);
}

template<typename InputIterator, typename OutputIterator>
inline OutputIterator oel::move(InputIterator first, InputIterator last, OutputIterator dest)
{
	return _detail::Copy(can_memmove_with(OutputIterator, InputIterator),
						 ::memmove,
						 std::make_move_iterator(first), std::make_move_iterator(last), dest);
}

template<typename SizedInRange, typename SizedOutRange>
inline auto oel::copy(const SizedInRange & src, SizedOutRange & dest) -> decltype(begin(dest))
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

namespace oel
{
namespace _detail
{
	template<typename T, typename Range> inline
	bool IdxValid(std::false_type, const Range & ib, T idx)
	{
		return as_unsigned(idx) < as_unsigned(oel::count(ib));
	}

	template<typename T, typename Range> inline
	bool IdxValid(std::true_type, const Range & ib, T idx)
	{
		return 0 <= idx && idx < oel::count(ib);
	}
}
}

template<typename Integer, typename CountableRange>
inline bool oel::index_valid(const CountableRange & ci, Integer idx)
{
	return _detail::IdxValid(bool_constant< std::is_signed<Integer>::value && std::numeric_limits<Integer>::digits < 47 >(),
							 ci, idx);
}


template<size_t I, typename T, size_t N>
inline T & oel::get(T(&arr)[N])
{
	static_assert(I < N, "Invalid array index");
	return arr[I];
}
template<size_t I, typename T, size_t N>
inline const T & oel::get(const T(&arr)[N])
{
	static_assert(I < N, "Invalid array index");
	return arr[I];
}

////////////////////////////////////////////////////////////////////////////////

namespace oel
{
namespace _detail
{
	template<typename T, typename... Args> inline
	T * New(std::true_type, Args &&... args)
	{
		return new T(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args> inline
	T * New(std::false_type, Args &&... args)
	{
		return new T{std::forward<Args>(args)...};
	}
}
}

template<typename T, typename... Args, typename>
inline std::unique_ptr<T> oel::make_unique(Args &&... args)
{
	auto idxU = static_cast<std::uint64_t>(idx);
	return idxU < as_unsigned(oel::count(r)); // assumes that r size is never greater than INT64_MAX
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename>
inline std::unique_ptr<T>  oel::make_unique(size_t arraySize)
{
	static_assert(std::extent<T>::value == 0, "make_unique forbids T[size]. Please use T[]");

#undef OEL_MAKE_UNIQUE_A

template<typename T, typename... ArgTs, typename>
inline void oel::set_new(std::unique_ptr<T> & up, ArgTs &&... args)
{
	up.reset( new T(std::forward<ArgTs>(args)...) );
}

template<typename T>
inline void oel::set_new(std::unique_ptr<T[]> & up, size_t arraySize)
{
	up.reset( new T[arraySize]() );
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename... Args>
std::array<T, sizeof...(Args)>  oel::make_array_static_cast(Args &&... args)
{
	return { static_cast<T>(std::forward<Args>(args))... };
}