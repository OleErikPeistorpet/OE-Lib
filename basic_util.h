#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "debug.h"

#include <iterator>


#if _MSC_VER && _MSC_VER < 1900
	#ifndef _ALLOW_KEYWORD_MACROS
		#define _ALLOW_KEYWORD_MACROS 1
	#endif

	#ifndef noexcept
		#define noexcept throw()
	#endif

	#undef constexpr
	#define constexpr

	#define OEL_ALIGNOF __alignof
#else
	#define OEL_ALIGNOF alignof
#endif


/// Obscure Efficient Library
namespace oel
{

using std::size_t;


using std::begin;
using std::end;

#if _MSC_VER || __GNUC__ >= 5
	using std::cbegin;
	using std::cend;
	using std::rbegin;
	using std::crbegin;
	using std::rend;
	using std::crend;
#else
	/// Equivalent to std::cbegin found in C++14
	template<typename Range> inline
	auto cbegin(const Range & r) -> decltype(std::begin(r))  { return std::begin(r); }
	/// Equivalent to std::cend found in C++14
	template<typename Range> inline
	auto cend(const Range & r) -> decltype(std::end(r))  { return std::end(r); }
#endif

/// Argument-dependent lookup non-member begin, defaults to std::begin
template<typename Range> inline
auto adl_begin(Range & r)       -> decltype(begin(r))  { return begin(r); }
/// Const version of adl_begin
template<typename Range> inline
auto adl_begin(const Range & r) -> decltype(begin(r))  { return begin(r); }

/// Argument-dependent lookup non-member end, defaults to std::end
template<typename Range> inline
auto adl_end(Range & r)       -> decltype(end(r))  { return end(r); }
/// Const version of adl_end
template<typename Range> inline
auto adl_end(const Range & r) -> decltype(end(r))  { return end(r); }

} // namespace oel

/** @code
	auto it = container.begin();     // Fails with types that don't have begin member such as built-in arrays
	auto it = std::begin(container); // Fails with types that only have non-member begin outside of namespace std
	using std::begin; auto it = begin(container); // Generic, good
	auto it = adl_begin(container);  // Generic, good
@endcode  */
using oel::adl_begin;
using oel::adl_end;

namespace oel
{

template<typename Iterator>
using difference_type = typename std::iterator_traits<Iterator>::difference_type;

/// Returns r.size() as signed (difference_type of iterator returned by begin(r))
template<typename SizedRange>
constexpr auto ssize(const SizedRange & r)
 -> decltype( r.size(), difference_type<decltype(begin(r))>() )  { return r.size(); }

/// Returns number of elements in array as signed type
template<typename T, std::ptrdiff_t Size>
constexpr std::ptrdiff_t ssize(const T (&)[Size]) noexcept  { return Size; }

/** @brief Count the elements in r
*
* @return ssize(r) if possible, otherwise equivalent to std::distance(begin(r), end(r))  */
template<typename InputRange>
auto count(const InputRange & r) -> difference_type<decltype(begin(r))>;



/// Convert iterator to pointer. This should be overloaded for each contiguous memory iterator class
template<typename T> inline
T * to_pointer_contiguous(T * ptr) noexcept  { return ptr; }

template<typename Iterator> inline
auto to_pointer_contiguous(std::move_iterator<Iterator> it) noexcept
 -> decltype( to_pointer_contiguous(it.base()) )  { return to_pointer_contiguous(it.base()); }


/// If an IteratorSource range can be copied to an IteratorDest range with memmove, is-a std::true_type, else false_type
template<typename IteratorDest, typename IteratorSource>
struct can_memmove_with;


template<bool Val>
using bool_constant = std::integral_constant<bool, Val>;


/// Equivalent to std::is_trivially_copyable, but can be specialized for a type if you are sure memcpy is safe to copy it
template<typename T>
struct is_trivially_copyable :
	#if __GLIBCXX__ && __GNUC__ == 4
		bool_constant< (__has_trivial_copy(T) && __has_trivial_assign(T))
			#if __INTEL_COMPILER
				|| __is_pod(T)
			#endif
			> {};
	#else
		std::is_trivially_copyable<T> {};
	#endif



/// For copy functions that return the end of both source and destination ranges
template<typename InIterator, typename OutIterator>
struct range_ends
{
	InIterator  src_end;
	OutIterator dest_end;
};



template<typename T>
using make_signed_t   = typename std::make_signed<T>::type;
template<typename T>
using make_unsigned_t = typename std::make_unsigned<T>::type;



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is implementation details


#if _MSC_VER
	template<typename T, size_t S> inline
	T *       to_pointer_contiguous(std::_Array_iterator<T, S> it)       { return it._Unchecked(); }
	template<typename T, size_t S> inline
	const T * to_pointer_contiguous(std::_Array_const_iterator<T, S> it) { return it._Unchecked(); }

	template<typename S> inline
	typename std::_String_iterator<S>::pointer  to_pointer_contiguous(std::_String_iterator<S> it)
	{
		return it._Unchecked();
	}
	template<typename S> inline
	typename std::_String_const_iterator<S>::pointer  to_pointer_contiguous(std::_String_const_iterator<S> it)
	{
		return it._Unchecked();
	}
#elif __GLIBCXX__
	template<typename T, typename U> inline
	T * to_pointer_contiguous(__gnu_cxx::__normal_iterator<T *, U> it) noexcept  { return it.base(); }
#endif


namespace _detail
{
	template<typename T>   // (target, source)
	is_trivially_copyable<T> CanMemmoveArrays(T *, const T *) { return {}; }

	template<typename IterDest, typename IterSrc>
	auto CanMemmoveWith(IterDest dest, IterSrc src)
	 -> decltype( _detail::CanMemmoveArrays(to_pointer_contiguous(dest), to_pointer_contiguous(src)) ) { return {}; }

	// SFINAE fallback for cases where to_pointer_contiguous(iterator) would be ill-formed or return types are not compatible
	inline std::false_type CanMemmoveWith(...) { return {}; }

////////////////////////////////////////////////////////////////////////////////

	template<typename SizedRange> inline // pass dummy int to prefer this overload
	auto Count(const SizedRange & r, int) -> decltype(oel::ssize(r)) { return oel::ssize(r); }

	template<typename InputRange> inline
	auto Count(const InputRange & r, long) -> decltype( std::distance(begin(r), end(r)) )
											   { return std::distance(begin(r), end(r)); }
}

} // namespace oel

/// @cond FALSE
template<typename IteratorDest, typename IteratorSource>
struct oel::can_memmove_with : decltype( _detail::CanMemmoveWith(std::declval<IteratorDest>(),
																 std::declval<IteratorSource>()) ) {};
/// @endcond


template<typename InputRange>
inline auto oel::count(const InputRange & r) -> difference_type<decltype(begin(r))>
{
	return _detail::Count(r, int{});
}