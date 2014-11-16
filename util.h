#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_range_util.h"

#include <algorithm>
#include <memory>
#include <cstring>
#include <cstdint>

/**
* @file util.h
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



#if CPP11_VARIADIC_TEMPL

/// Calls new T using constructor syntax with args as the parameter list and wraps it in a std::unique_ptr.
template<typename T, typename... Params, typename = std::enable_if_t<!std::is_array<T>::value> >
std::unique_ptr<T>  make_unique(Params &&... args);

/** @brief Calls new T using uniform initialization syntax (args in braces) and wraps it in a std::unique_ptr.
*
* Works for aggregate initialization of classes without any user-provided constructor.
* Constructors taking std::initializer_list are preferred during overload resolution  */
template<typename T, typename... Params>
std::unique_ptr<T> make_unique_brace(Params &&... args);

/** @brief Calls new T and wraps it in a std::unique_ptr. The T object is default-initialized.
*
* Default initialization of non-class T produces an object with indeterminate value  */
template<typename T> inline
std::unique_ptr<T> make_unique_default()
{
	static_assert(!std::is_array<T>::value, "Please use make_unique_default<T[]>(size_t) for array");
	return std::unique_ptr<T>(new T);
}

/// Calls new T[arraySize]() and wraps it in a std::unique_ptr. The array is value-initialized.
template< typename T, typename = std::enable_if_t<std::is_array<T>::value> >
std::unique_ptr<T> make_unique(size_t arraySize);

/** @brief Calls new T[arraySize] and wraps it in a std::unique_ptr. The array is default-initialized.
*
* Default initialization of non-class T produces objects with indeterminate value  */
template<typename T>
std::unique_ptr<T> make_unique_default(size_t arraySize);

#endif


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
void erase_back(Container & ctr, typename Container::iterator newEnd)  { ctr.erase(newEnd, ctr.end()); }

/**
* @brief Erase consecutive duplicate elements in container.
*
* To erase duplicates anywhere, sort container contents first. (Or just use std::set or unordered_set)  */
template<class Container>
void erase_successive_dup(Container & ctr);



/// Create a std::move_iterator from InputIterator
template<typename InputIterator> inline
std::move_iterator<InputIterator> make_move_iter(InputIterator it)  { return std::make_move_iterator(it); }


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
* The ranges shall not overlap. To move instead of copy, pass make_move_iter(first)  */
template<typename InputIterator, typename Count, typename OutputIterator>
range_ends<InputIterator, OutputIterator>  copy_nonoverlap(InputIterator first, Count count, OutputIterator dest);

/// Same as std::copy, but much faster for arrays of simple structs/classes than Visual C++ version
template<typename InputIterator, typename OutputIterator>
OutputIterator copy(InputIterator first, InputIterator last, OutputIterator dest);

/// Same as std::move(InIterator, InIterator, OutIterator), but much faster for arrays of simple classes than VC++ version
template<typename InputIterator, typename OutputIterator>
OutputIterator move(InputIterator first, InputIterator last, OutputIterator dest);

/**
* @brief Copies count elements from range beginning at first into the range beginning at dest
* @return a struct with iterators to the end of both ranges
*
* To move instead of copy, pass make_move_iter(first)  */
template<typename InputIterator, typename Count, typename OutputIterator>
range_ends<InputIterator, OutputIterator> copy_n(InputIterator first, Count count, OutputIterator dest);



///
template<typename Count, typename T, typename InputIterator>
Count find_idx(InputIterator first, Count count, const T & value);
///
template<typename T, typename Range> inline
auto find_idx(const Range & toSearch, const T & value) -> decltype(count(toSearch))
{
	return oetl::find_idx(begin(toSearch), count(toSearch), value);
}

///
template<typename T, typename BidirectionRange>
auto rfind_idx(const BidirectionRange & toSearch, const T & value) -> decltype(count(toSearch))
{
	auto pos = count(toSearch);
	auto it = end(toSearch);
	while (--pos != decltype(pos)(-1))
	{
		--it;
		if (*it == value)
			break;
	}
	return pos;
}


///
template<typename ForwardRange, typename T>
auto find_sorted(ForwardRange & range, const T & val) -> decltype(begin(range));
///
template<typename ForwardRange, typename T, typename Compare>
auto find_sorted(ForwardRange & range, const T & val, Compare comp) -> decltype(begin(range));


template<typename BidirectionRange, typename Func> inline
Func for_each_reverse(BidirectionRange && range, Func func)
{	// perform function while it returns true for each element in reverse
	auto const first = begin(range);
	auto it = end(range);
	while (first != it && func(*(--it)));

	return func;
}


////////////////////////////////////////////////////////////////////////////////

// Implementation only in rest of the file


/// Type trait to check whether a type is integral and has size 1
template<typename T>
struct is_byte : std::integral_constant< bool,
		std::is_integral<T>::value && sizeof(T) == 1 > {};


namespace _detail
{
	template<class Container> inline
	auto EraseSuccessiveDup(Container & ctr, int) -> decltype(ctr.unique()) { return ctr.unique(); }

	template<class Container> inline
	void EraseSuccessiveDup(Container & ctr, long)
	{
		erase_back( ctr, std::unique(begin(ctr), end(ctr)) );
	}

////////////////////////////////////////////////////////////////////////////////

	template<typename InputIter, typename OutputIter, typename Unused> inline
	OutputIter Copy(std::false_type, Unused, InputIter first, InputIter last, OutputIter dest)
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
#	if OETL_MEM_BOUND_DEBUG_LVL >= 2
		if (0 != count)
		{	// Dereference iterators at bounds, this detects out of range errors if they are checked iterators
			*first; *dest;
			*(dest + (count - 1));
		}
#	endif
		doCopy(to_ptr(dest), to_ptr(first), count * sizeof(*first));
		return dest + count;
	}

	template<typename IterSrc, typename Count, typename IterDest, typename CopyFunc> inline
	range_ends<IterSrc, IterDest> CopyN(std::true_type, CopyFunc doCopy,
										IterSrc first, Count count, IterDest dest)
	{	// can use memcpy/memmove
		if (0 < count)
		{
#		if OETL_MEM_BOUND_DEBUG_LVL >= 2
			*(first + (count - 1));        // Dereference iterators at bounds, this detects
			*dest; *(dest + (count - 1));  // out of range errors if they are checked iterators
#		endif
			doCopy(to_ptr(dest), to_ptr(first), count * sizeof(*first));
			first += count;
			dest += count;
		}
		return {first, dest};
	}

	template<typename InputIter, typename Count, typename OutputIter, typename Unused> inline
	range_ends<InputIter, OutputIter> CopyN(std::false_type, Unused, InputIter first, Count count, OutputIter dest)
	{
		for (; 0 < count; --count)
		{
			*dest = *first;
			++dest; ++first;
		}
		return {first, dest};
	}
} // namespace _detail

} // namespace oetl

template<typename InputIterator, typename OutputIterator>
inline OutputIterator oetl::copy(InputIterator first, InputIterator last, OutputIterator dest)
{
	return _detail::Copy(can_memmove_ranges_with(dest, begin(source)),
						 memmove,
						 begin(source), end(source), dest);
}

template<typename InputRange, typename OutputIterator>
inline OutputIterator oetl::copy_nonoverlap(const InputRange & source, OutputIterator dest)
{
	return _detail::Copy(can_memmove_ranges_with(dest, begin(source)),
						 memcpy,
						 begin(source), end(source), dest);
}

template<typename InputIterator, typename Count, typename OutputIterator>
inline oetl::range_ends<InputIterator, OutputIterator>  oetl::copy_n(InputIterator first, Count count, OutputIterator dest)
{
	return _detail::CopyN(can_memmove_ranges_with(dest, first), memmove,
						  first, count, dest);
}

template<typename InputIterator, typename Count, typename OutputIterator>
inline oetl::range_ends<InputIterator, OutputIterator>  oetl::
	copy_nonoverlap(InputIterator first, Count count, OutputIterator dest)
{
	return _detail::CopyN(can_memmove_ranges_with(dest, first), memcpy,
						  first, count, dest);
}

template<typename InputIterator, typename OutputIterator>
inline OutputIterator  oetl::move(InputIterator first, InputIterator last, OutputIterator dest)
{
	return oetl::copy(make_move_iter(first), make_move_iter(last), dest);
}

////////////////////////////////////////////////////////////////////////////////

template<class Container>
void oetl::erase_successive_dup(Container & ctr)
{
	_detail::EraseSuccessiveDup(ctr, int{});
}

////////////////////////////////////////////////////////////////////////////////

namespace smsc
{ namespace _detail
  {
	template<typename Size, typename CntigusIter> inline
	Size FindIdx(CntigusIter first, Size count, int val, std::true_type)
	{	// contiguous mem iterator with integral value_type of size 1
		auto const pBuf = to_ptr(first);
		auto found = static_cast<decltype(pBuf)>(memchr(pBuf, val, count));
		return found ?
		       static_cast<Size>(found - pBuf) :
		       Size(-1);
	}

	template<typename Size, typename T, typename InputIter> inline
	Size FindIdx(InputIter first, Size count, const T & val, std::false_type)
	{
		for (Size i = 0; i < count; ++i)
		{
			if (*first == val)
				return i;

			++first;
		}
		return Size(-1);
	}
  }
}

template<typename Count, typename T, typename InputIterator>
inline Count oetl::find_idx(InputIterator first, Count count, const T & value)
{
	std::integral_constant< bool,
			oetl::is_byte< typename std::iterator_traits<InputIterator>::value_type >::value
			&& !std::is_same<T, bool>::value
			&& is_contiguous_iterator<InputIterator>::value
						  > canUseMemchr;
	return _detail::FindIdx(first, count, value, canUseMemchr);
}


template<typename ForwardRange, typename T>
inline auto oetl::find_sorted(ForwardRange & range, const T & val) -> decltype(begin(range))
{
	return find_sorted(range, val, std::less<>{});
}

template<typename ForwardRange, typename T, typename Compare>
inline auto oetl::find_sorted(ForwardRange & range, const T & val, Compare comp) -> decltype(begin(range))
{
	auto const last = end(range);
	// Finds the lower bound in at most log(last - begin) + 1 comparisons
	auto const it = std::lower_bound(begin(range), last, val, comp);

	if (last != it && !comp(val, *it))
		return it; // found
	else
		return last; // not found
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

////////////////////////////////////////////////////////////////////////////////

#if CPP11_VARIADIC_TEMPL

template<typename T, typename... Params, typename>
std::unique_ptr<T>  oetl::make_unique(Params &&... args)
{
	T * p = new T(std::forward<Params>(args)...); // direct-initialize, or value-initialize if no args
	return std::unique_ptr<T>(p);
}

template<typename T, typename... Params>
inline std::unique_ptr<T>  oetl::make_unique_brace(Params &&... args)
{
	static_assert(!std::is_array<T>::value, "make_unique_brace forbids array of T");

	T * p = new T{std::forward<Params>(args)...}; // list-initialize
	return std::unique_ptr<T>(p);
}

template<typename T, typename>
inline std::unique_ptr<T>  oetl::make_unique(size_t arraySize)
{
	static_assert(std::extent<T>::value == 0, "make_unique forbids T[size]. Please use T[]");

	typedef typename std::remove_extent<T>::type Elem;
	return std::unique_ptr<T>( new Elem[arraySize]() ); // value-initialize
}

template<typename T>
inline std::unique_ptr<T>  oetl::make_unique_default(size_t arraySize)
{
	static_assert(std::is_array<T>::value, "make_unique_default(size_t arraySize) requires T[]");
	static_assert(std::extent<T>::value == 0, "make_unique_default forbids T[size]. Please use T[]");

	typedef typename std::remove_extent<T>::type Elem;
	return std::unique_ptr<T>(new Elem[arraySize]);  // default-initialize
}
#endif
