#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#include <algorithm>
#include <memory>
#include <limits>
#include <array>
#include <string.h>

/**
* @file util.h
* @brief Utilities, including algorithms
*
* Designed to interface with the standard library. Contains erase functions, copy functions, make_unique and more.
*/

namespace oetl
{

/// Given argument val of integral or enumeration type T, returns val cast to the signed integer type corresponding to T
template<typename T> inline
typename std::make_signed<T>::type    as_signed(T val) NOEXCEPT    { return typename std::make_signed<T>::type(val); }
/// Given argument val of integral or enumeration type T, returns val cast to the unsigned integer type corresponding to T
template<typename T> inline
typename std::make_unsigned<T>::type  as_unsigned(T val) NOEXCEPT  { return typename std::make_unsigned<T>::type(val); }



/// Check if index is valid (can be used with operator[]) for array or other iterable.
template<typename Integer, typename CountableIterable>
bool index_valid(const CountableIterable & ci, Integer index);


template<size_t I, typename T, size_t N>
T &       get(T (&arr)[N]);

template<size_t I, typename T, size_t N>
const T & get(const T (&arr)[N]);


struct default_array_type_t;

template<typename T, typename...>
struct deduce_array_type
{
	using type = T;
};

template<typename... T>
struct deduce_array_type<default_array_type_t, T...>
{
	using type = typename std::decay< typename std::common_type<T...>::type >::type;
};

template<typename T = default_array_type_t, typename... Params>
auto make_array(Params &&... args)
 -> std::array<typename deduce_array_type<T, Params...>::type, sizeof...(Params)>
{
	using U = typename deduce_array_type<T, Params...>::type;
	return { static_cast<U>(std::forward<Params>(args))... };
}



#if _MSC_VER

using std::make_unique;

#else

/// Calls new T using constructor syntax with args as the parameter list and wraps it in a std::unique_ptr.
template<typename T, typename... Params, typename = std::enable_if_t<!std::is_array<T>::value> >
std::unique_ptr<T>  make_unique(Params &&... args);

/// Calls new T[arraySize]() and wraps it in a std::unique_ptr. The array is value-initialized.
template< typename T, typename = std::enable_if_t<std::is_array<T>::value> >
std::unique_ptr<T> make_unique(size_t arraySize);

#endif

/** @brief Calls new T using uniform initialization syntax (args in braces) and wraps it in a std::unique_ptr.
*
* Works for aggregate initialization of classes without any user-provided constructor.
* Constructors taking std::initializer_list are preferred during overload resolution  */
template<typename T, typename... Params>
std::unique_ptr<T> make_unique_brace(Params &&... args);

/** @brief Calls new T and wraps it in a std::unique_ptr. The T object is default-initialized.
*
* Default initialization of non-class T produces an object with indeterminate value  */
template<typename T>
std::unique_ptr<T> make_unique_default();

/** @brief Calls new T[arraySize] and wraps it in a std::unique_ptr. The array is default-initialized.
*
* Default initialization of non-class T produces objects with indeterminate value  */
template<typename T>
std::unique_ptr<T> make_unique_default(size_t arraySize);

/// Calls new T using constructor syntax with args as the parameter list and wraps it in a std::unique_ptr.
template<typename T, typename... Params, typename = std::enable_if_t<!std::is_array<T>::value> >
void set_new(std::unique_ptr<T> & ptr, Params &&... args);

/// Calls new T[arraySize]() and wraps it in a std::unique_ptr. The array is value-initialized.
template<typename T>
void set_new(std::unique_ptr<T[]> & ptr, size_t arraySize);


/**
* @brief Erase the element at index from subscriptable without maintaining order of elements.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template<class PopBackSubscriptable, typename Index> inline
void erase_unordered(PopBackSubscriptable & ps, Index index)
{
	ps[index] = std::move(ps.back());
	ps.pop_back();
}
/**
* @brief Erase the element at position from iterable without maintaining order of elements.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template<class PopBackIterable, typename OutputIterator> inline
void erase_unordered(PopBackIterable & ib, OutputIterator position)
{
	*position = std::move(ib.back());
	ib.pop_back();
}

/**
* @brief Erase consecutive duplicate elements in iterable.
*
* By sorting contents first, all duplicates will be erased. */
template<class EraseIterable>
void erase_successive_dup(EraseIterable & ei);


/**
* @brief Copies the elements in source into the Iterable beginning at dest
* @return an iterator to the end of the destination
*
* The memory shall not overlap. To move instead of copy, include iterator_range.h and pass move_range(source)  */
template<typename InputIterable, typename OutputIterator>
OutputIterator copy_nonoverlap(const InputIterable & source, OutputIterator dest);
/**
* @brief Copies count elements from Iterable beginning at first into the Iterable beginning at dest
* @return a struct with iterators to the end of both source and destination
*
* The memory shall not overlap. To move instead of copy, pass make_move_iter(first)  */
template<typename InputIterator, typename Count, typename OutputIterator>
end_iterators<InputIterator, OutputIterator>  copy_nonoverlap(InputIterator first, Count count, OutputIterator dest);

/// Same as std::copy, but much faster for arrays of simple structs/classes than Visual C++ version
template<typename InputIterator, typename OutputIterator>
OutputIterator copy(InputIterator first, InputIterator last, OutputIterator dest);

/// Same as std::move(InIterator, InIterator, OutIterator), but much faster for arrays of simple classes than VC++ version
template<typename InputIterator, typename OutputIterator>
OutputIterator move(InputIterator first, InputIterator last, OutputIterator dest);

/**
* @brief Copies count elements from Iterable beginning at first into the Iterable beginning at dest
* @return a struct with iterators to the end of both source and destination
*
* To move instead of copy, pass make_move_iter(first)  */
template<typename InputIterator, typename Count, typename OutputIterator>
end_iterators<InputIterator, OutputIterator> copy_n(InputIterator first, Count count, OutputIterator dest);



template<typename T>
struct identity { using type = T; };

template<typename T>
using identity_t = typename identity<T>::type;

/**
* @brief Bring the value into the range [lo, hi].
*
* @return If val is less than lo, return lo. If val is greater than hi, return hi. Otherwise, return val. */
template<typename T>
const T & clamp(const T & val, const identity_t<T> & lo,
							   const identity_t<T> & hi)  { return val < lo ? lo : (hi < val ? hi : val); }



///
template<typename Count, typename T, typename InputIterator>
Count find_idx(InputIterator first, Count count, const T & value);
///
template<typename T, typename InputIterable> inline
auto find_idx(const InputIterable & toSearch, const T & value) -> difference_type<decltype(begin(toSearch))>
{
	return oetl::find_idx(begin(toSearch), oetl::count(toSearch), value);
}

///
template<typename T, typename BidirectionIterable> inline
auto rfind_idx(const BidirectionIterable & toSearch, const T & value) -> difference_type<decltype(begin(toSearch))>
{
	auto pos = oetl::count(toSearch);
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
template<typename ForwardIterable, typename T>
auto find_sorted(ForwardIterable & ib, const T & val) -> decltype(begin(ib));
///
template<typename ForwardIterable, typename T, typename Compare>
auto find_sorted(ForwardIterable & ib, const T & val, Compare comp) -> decltype(begin(ib));


/// Functor for operator== with operator * on arguments
template<class Derefable = void>
struct equal_to_deref
{
	bool operator()(const Derefable & a, const Derefable & b) const  { return *a == *b; }
};
/// Functor for operator < with operator * on arguments
template<class Derefable = void>
struct less_deref
{
	bool operator()(const Derefable & a, const Derefable & b) const  { return *a < *b; }
};
/// Transparent functor for operator== with operator * on arguments
template<> struct equal_to_deref<void>
{
	template<class T, class U>
	bool operator()(const T & a, const U & b) const  { return *a == *b; }
};
/// Transparent functor for operator < with operator * on arguments
template<> struct less_deref<void>
{
	template<class T, class U>
	bool operator()(const T & a, const U & b) const  { return *a < *b; }
};



template<typename BidirectionIterable, typename Func> inline
Func for_each_reverse(BidirectionIterable && ib, Func func)
{	// perform function while it returns true for each element in reverse
	auto const first = begin(ib);
	auto it = end(ib);
	while (first != it && func(*(--it)));

	return func;
}


////////////////////////////////////////////////////////////////////////////////

// Implementation only in rest of the file


/// @cond INTERNAL
namespace _detail
{
	template<class HasUnique> inline
	auto EraseSuccessiveDup(HasUnique & ei, int) -> decltype(ei.unique()) { return ei.unique(); }

	template<class EraseIterable>
	void EraseSuccessiveDup(EraseIterable & ei, long)
	{
		erase_back( ei, std::unique(begin(ei), end(ei)) );
	}

////////////////////////////////////////////////////////////////////////////////

	template<typename InputItor, typename Sentinel, typename OutputItor, typename Unused> inline
	OutputItor Copy(std::false_type, Unused, InputItor first, Sentinel last, OutputItor dest)
	{
		while (first != last)
		{
			*dest = *first;
			++dest; ++first;
		}
		return dest;
	}

	template<typename ItorSrc, typename ItorDest, typename TrivialCopyFunc> inline
	ItorDest Copy(std::true_type, TrivialCopyFunc doCopy,
				  ItorSrc const first, ItorSrc const last, ItorDest const dest)
	{	// can use memcpy/memmove
		auto const count = last - first;
#	if OETL_MEM_BOUND_DEBUG_LVL
		if (0 != count)
		{	// Dereference iterators at bounds, this detects out of range errors if they are checked iterators
		OETL_PUSH_IGNORE_UNUSED_VALUE
			*first; *dest;
			*(dest + (count - 1));
		OETL_POP_DIAGNOSTIC
		}
#	endif
		doCopy(to_ptr(dest), to_ptr(first), count * sizeof(*first));
		return dest + count;
	}

	template<typename ItorSrc, typename Count, typename ItorDest, typename CopyFunc> inline
	end_iterators<ItorSrc, ItorDest> CopyN(std::true_type, CopyFunc doCopy,
										   ItorSrc first, Count count, ItorDest dest)
	{	// can use memcpy/memmove
		if (0 < count)
		{
#		if OETL_MEM_BOUND_DEBUG_LVL
			OETL_PUSH_IGNORE_UNUSED_VALUE
			*(first + (count - 1));        // Dereference iterators at bounds, this detects
			*dest; *(dest + (count - 1));  // out of range errors if they are checked iterators
			OETL_POP_DIAGNOSTIC
#		endif
			doCopy(to_ptr(dest), to_ptr(first), count * sizeof(*first));
			first += count;
			dest += count;
		}
		return {first, dest};
	}

	template<typename InputItor, typename Count, typename OutputItor, typename Unused> inline
	end_iterators<InputItor, OutputItor> CopyN(std::false_type, Unused, InputItor first, Count count, OutputItor dest)
	{
		for (; 0 < count; --count)
		{
			*dest = *first;
			++dest; ++first;
		}
		return {first, dest};
	}
} // namespace _detail
/// @endcond

} // namespace oetl

template<typename InputIterator, typename OutputIterator>
inline OutputIterator oetl::copy(InputIterator first, InputIterator last, OutputIterator dest)
{
	return _detail::Copy(can_memmove_ranges_with(dest, first),
						 ::memmove,
						 first, last, dest);
}

template<typename InputIterable, typename OutputIterator>
inline OutputIterator oetl::copy_nonoverlap(const InputIterable & source, OutputIterator dest)
{
	return _detail::Copy(can_memmove_ranges_with(dest, begin(source)),
						 ::memcpy,
						 begin(source), end(source), dest);
}

template<typename InputIterator, typename Count, typename OutputIterator>
inline oetl::end_iterators<InputIterator, OutputIterator>  oetl::copy_n(InputIterator first, Count count, OutputIterator dest)
{
	return _detail::CopyN(can_memmove_ranges_with(dest, first), ::memmove,
						  first, count, dest);
}

template<typename InputIterator, typename Count, typename OutputIterator>
inline oetl::end_iterators<InputIterator, OutputIterator>  oetl::
	copy_nonoverlap(InputIterator first, Count count, OutputIterator dest)
{
	return _detail::CopyN(can_memmove_ranges_with(dest, first), ::memcpy,
						  first, count, dest);
}

template<typename InputIterator, typename OutputIterator>
inline OutputIterator  oetl::move(InputIterator first, InputIterator last, OutputIterator dest)
{
	return oetl::copy(make_move_iter(first), make_move_iter(last), dest);
}

////////////////////////////////////////////////////////////////////////////////

template<class EraseIterable>
inline void oetl::erase_successive_dup(EraseIterable & ei)
{
	_detail::EraseSuccessiveDup(ei, int{});
}

////////////////////////////////////////////////////////////////////////////////

namespace oetl
{
namespace _detail
{
	template<typename Size, typename CntigusItor> inline
	Size FindIdx(CntigusItor first, Size count, int val, std::true_type)
	{	// contiguous mem iterator with integral value_type of size 1
		auto const pBuf = to_ptr(first);
		auto found = static_cast<decltype(pBuf)>(memchr(pBuf, val, count));
		return found ?
		       static_cast<Size>(found - pBuf) :
		       Size(-1);
	}

	template<typename Size, typename T, typename InputItor> inline
	Size FindIdx(InputItor first, Size count, const T & val, std::false_type)
	{
		for (Size i = 0; i < count; ++i)
		{
			if (*first == val)
				return i;

			++first;
		}
		return Size(-1);
	}


	template<typename T>
	struct IsSz1Int : bool_constant<
			std::is_integral<T>::value && sizeof(T) == 1 > {};
}

/// If memchr can be used, returns std::true_type, else false_type
template<typename Iterator, typename T> inline
auto can_memchr_with(Iterator it, const T &)
 -> decltype( to_ptr(it),
			  bool_constant< _detail::IsSz1Int< typename std::iterator_traits<Iterator>::value_type >::value
							 && !std::is_same<T, bool>::value >() )  { return {}; }

// SFINAE fallback
inline std::false_type can_memchr_with(...)  { return {}; }

} // namespace oetl

template<typename Count, typename T, typename InputIterator>
inline Count oetl::find_idx(InputIterator first, Count count, const T & value)
{
	return _detail::FindIdx(first, count, value, can_memchr_with(first, value));
}


template<typename ForwardIterable, typename T>
inline auto oetl::find_sorted(ForwardIterable & ib, const T & val) -> decltype(begin(ib))
{
	return find_sorted(ib, val, std::less<>{});
}

template<typename ForwardIterable, typename T, typename Compare>
inline auto oetl::find_sorted(ForwardIterable & ib, const T & val, Compare comp) -> decltype(begin(ib))
{
	auto const last = end(ib);
	// Finds the lower bound in at most log(last - begin) + 1 comparisons
	auto const it = std::lower_bound(begin(ib), last, val, comp);

	if (last != it && !comp(val, *it))
		return it; // found
	else
		return last; // not found
}

////////////////////////////////////////////////////////////////////////////////

namespace oetl
{
  namespace _detail
  {
	template<typename T, typename Iterable> inline
	bool IdxValid(std::false_type, const Iterable & ib, T idx)
	{
		return as_unsigned(idx) < as_unsigned(oetl::count(ib));
	}

	template<typename T, typename Iterable> inline
	bool IdxValid(std::true_type, const Iterable & ib, T idx)
	{
		return 0 <= idx && idx < oetl::count(ib);
	}
  }
}

template<typename Integer, typename CountableIterable>
inline bool oetl::index_valid(const CountableIterable & ci, Integer idx)
{
	using LimUChar = std::numeric_limits<unsigned char>;
	return _detail::IdxValid(bool_constant< std::is_signed<Integer>::value && (sizeof(Integer) * LimUChar::digits < 60) >(),
							 ci, idx);
}


template<size_t I, typename T, size_t N>
inline T & oetl::get(T (&arr)[N])
{
	static_assert(I < N, "Invalid array index");
	return a[I];
}

template<size_t I, typename T, size_t N>
inline const T & oetl::get(const T (&arr)[N])
{
	static_assert(I < N, "Invalid array index");
	return a[I];
}

////////////////////////////////////////////////////////////////////////////////

#if !_MSC_VER

template<typename T, typename... Params, typename>
inline std::unique_ptr<T>  oetl::make_unique(Params &&... args)
{
	T * p = new T(std::forward<Params>(args)...); // direct-initialize, or value-initialize if no args
	return std::unique_ptr<T>(p);
}

template<typename T, typename>
inline std::unique_ptr<T>  oetl::make_unique(size_t arraySize)
{
	static_assert(std::extent<T>::value == 0, "make_unique forbids T[size]. Please use T[]");

	using Elem = typename std::remove_extent<T>::type;
	return std::unique_ptr<T>( new Elem[arraySize]() ); // value-initialize
}

#endif

template<typename T, typename... Params>
inline std::unique_ptr<T>  oetl::make_unique_brace(Params &&... args)
{
	static_assert(!std::is_array<T>::value, "make_unique_brace forbids array of T");

	T * p = new T{std::forward<Params>(args)...}; // list-initialize
	return std::unique_ptr<T>(p);
}

template<typename T>
inline std::unique_ptr<T>  oetl::make_unique_default()
{
	static_assert(!std::is_array<T>::value, "Please use make_unique_default<T[]>(size_t) for array");
	return std::unique_ptr<T>(new T);
}

template<typename T>
inline std::unique_ptr<T>  oetl::make_unique_default(size_t arraySize)
{
	static_assert(std::is_array<T>::value, "make_unique_default(size_t arraySize) requires T[]");
	static_assert(std::extent<T>::value == 0, "make_unique_default forbids T[size]. Please use T[]");

	using Elem = typename std::remove_extent<T>::type;
	return std::unique_ptr<T>(new Elem[arraySize]);  // default-initialize
}

template<typename T, typename... Params, typename>
inline void oetl::set_new(std::unique_ptr<T> & up, Params &&... args)
{
	up.reset( new T(std::forward<Params>(args)...) );
}

template<typename T>
inline void oetl::set_new(std::unique_ptr<T[]> & up, size_t arraySize)
{
	up.reset( new T[arraySize]() );
}