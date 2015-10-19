#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "user_traits.h"

#include <iterator>


#if !defined(NDEBUG) && !defined(OEL_MEM_BOUND_DEBUG_LVL)
/** @brief Undefined: no array index and iterator checks. 1: most debug checks. 2: all checks, often slow.
*
* Warning: Undefined (by NDEBUG defined) is not binary compatible with levels 1 and 2. */
	#define OEL_MEM_BOUND_DEBUG_LVL 2
#endif


#if _MSC_VER
	#define OEL_CONST_COND __pragma(warning(suppress : 4127))
#else
	#define OEL_CONST_COND
#endif


#ifndef OEL_HALT
	/// Do not throw an exception from OEL_HALT, since it is used in noexcept functions
	#if _MSC_VER
		#define OEL_HALT() __debugbreak()
	#else
		#define OEL_HALT() __asm__("int $3")
	#endif
#endif

#ifndef ASSERT_ALWAYS_NOEXCEPT
	/// Standard assert implementations typically don't break on the line of the assert, so we roll our own
	#define ASSERT_ALWAYS_NOEXCEPT(expr)  \
		OEL_CONST_COND  \
		do {  \
			if (!(expr)) OEL_HALT();  \
		} while (false)
#endif


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
	using std::begin; auto it = begin(container); // Argument-dependent lookup, as generic as it gets
	auto it = adl_begin(container);  // Equivalent to line above
@endcode  */
using oel::adl_begin;
using oel::adl_end;

namespace oel
{

/// Returns r.size() as signed (SizedRange::difference_type)
template<typename SizedRange> inline
constexpr auto ssize(const SizedRange & r)
 -> decltype( static_cast<typename SizedRange::difference_type>(r.size()) )
	 { return static_cast<typename SizedRange::difference_type>(r.size()); }
/// Returns number of elements in array as signed type
template<typename T, std::ptrdiff_t Size> inline
constexpr std::ptrdiff_t ssize(const T (&)[Size]) noexcept  { return Size; }

/** @brief Count the elements in r
*
* @return ssize(r) if possible, otherwise equivalent to std::distance(begin(r), end(r))  */
template<typename InputRange>
auto count(const InputRange & r) -> typename std::iterator_traits<decltype(begin(r))>::difference_type;



/// Convert iterator to pointer. This should be overloaded for each contiguous memory iterator class
template<typename T> inline
T * to_pointer_contiguous(T * ptr) noexcept  { return ptr; }

template<typename Iterator> inline
auto to_pointer_contiguous(std::move_iterator<Iterator> it) noexcept
 -> decltype( to_pointer_contiguous(it.base()) )  { return to_pointer_contiguous(it.base()); }


/// If an IteratorSource range can be copied to an IteratorDest range with memmove, is-a std::true_type, else false_type
template<typename IteratorDest, typename IteratorSource>
struct can_memmove_with;



/// Tag to specify default initialization. The const instance default_init is provided for convenience
struct default_init_tag
{	explicit default_init_tag() {}
}
const default_init;

/// Tag to select construction from a single range object. The const instance from_range is provided to pass
struct from_range_tag
{	explicit from_range_tag() {}
}
const from_range;



/// For copy functions that return the end of both source and destination ranges
template<typename InIterator, typename OutIterator>
struct range_ends
{
	InIterator  src_end;
	OutIterator dest_end;
};



/// Exists in std with C++14
template<bool Condition>
using enable_if_t = typename std::enable_if<Condition>::type;



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


#if OEL_MEM_BOUND_DEBUG_LVL
	#define OEL_MEM_BOUND_ASSERT  ASSERT_ALWAYS_NOEXCEPT
#else
	#define OEL_MEM_BOUND_ASSERT(expr) ((void) 0)
#endif

////////////////////////////////////////////////////////////////////////////////

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
	is_trivially_copyable<T> CanMemmoveArrays(T *, const T *);

	template<typename IterDest, typename IterSrc>
	auto CanMemmoveWith(IterDest dest, IterSrc src)
	 -> decltype( _detail::CanMemmoveArrays(to_pointer_contiguous(dest), to_pointer_contiguous(src)) );

	// SFINAE fallback for cases where to_pointer_contiguous(iterator) would be ill-formed or return types are not compatible
	inline std::false_type CanMemmoveWith(...);

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

template<typename T>
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};


template<typename InputRange>
inline auto oel::count(const InputRange & r) -> typename std::iterator_traits<decltype(begin(r))>::difference_type
{
	return _detail::Count(r, int{});
}