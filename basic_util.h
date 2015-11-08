#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "user_traits.h"

#include <iterator>


/** @file
*/

#if !defined(NDEBUG) && !defined(OEL_MEM_BOUND_DEBUG_LVL)
/** @brief Undefined/0: no array index and iterator checks. 1: most debug checks. 2: all checks, often slow.
*
* Warning: 0 (or undefined by NDEBUG defined) is not binary compatible with levels 1 and 2. */
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

#ifndef ALWAYS_ASSERT_NOEXCEPT
	/// Standard assert implementations typically don't break on the line of the assert, so we roll our own
	#define ALWAYS_ASSERT_NOEXCEPT(expr)  \
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

#if __cplusplus >= 201402L || _MSC_VER || __GNUC__ >= 5
	using std::cbegin;
	using std::cend;
	using std::crbegin;
	using std::crend;
#else
	/// Equivalent to std::cbegin found in C++14
	template<typename Range> inline
	constexpr auto cbegin(const Range & r) -> decltype(std::begin(r))  { return std::begin(r); }
	/// Equivalent to std::cend found in C++14
	template<typename Range> inline
	constexpr auto cend(const Range & r) -> decltype(std::end(r))      { return std::end(r); }
#endif

/** @brief Argument-dependent lookup non-member begin, defaults to std::begin
*
* Note the global using-directive  @code
	auto it = container.begin();     // Fails with types that don't have begin member such as built-in arrays
	auto it = std::begin(container); // Fails with types that have only non-member begin outside of namespace std
	// Argument-dependent lookup, as generic as it gets
	using std::begin; auto it = begin(container);
	auto it = adl_begin(container);  // Equivalent to line above
@endcode  */
template<typename Range> inline
constexpr auto adl_begin(Range & r) -> decltype(begin(r))         { return begin(r); }
/// Const version of adl_begin
template<typename Range> inline
constexpr auto adl_cbegin(const Range & r) -> decltype(begin(r))  { return begin(r); }

/// Argument-dependent lookup non-member end, defaults to std::end
template<typename Range> inline
constexpr auto adl_end(Range & r) -> decltype(end(r))         { return end(r); }
/// Const version of adl_end
template<typename Range> inline
constexpr auto adl_cend(const Range & r) -> decltype(end(r))  { return end(r); }

} // namespace oel

using oel::adl_begin;
using oel::adl_cbegin;
using oel::adl_end;
using oel::adl_cend;

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



/// Convert iterator to pointer. This should be overloaded for each contiguous memory iterator class
template<typename T> inline
T * to_pointer_contiguous(T * ptr) noexcept  { return ptr; }

template<typename Iterator> inline
auto to_pointer_contiguous(std::move_iterator<Iterator> it) noexcept
 -> decltype( to_pointer_contiguous(it.base()) )  { return to_pointer_contiguous(it.base()); }


/// If an IteratorSource range can be copied to an IteratorDest range with memmove, is-a std::true_type, else false_type
template<typename IteratorDest, typename IteratorSource>
struct can_memmove_with;



/// Tag to select a constructor that allocates a minimum amount of storage
struct reserve_tag
{	explicit reserve_tag() {}
}
const reserve; ///< An instance of reserve_tag to pass

/// Tag to specify default initialization
struct default_init_tag
{	explicit default_init_tag() {}
}
const default_init; ///< An instance of default_init_tag to pass

/// Tag to select construction from a single range object
struct from_range_tag
{	explicit from_range_tag() {}
}
const from_range; ///< An instance of from_range_tag to pass



/// Exists in std with C++14
template<bool Condition>
using enable_if_t = typename std::enable_if<Condition>::type;



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


#if defined(_CPPUNWIND) || defined(__EXCEPTIONS)
	#define OEL_THROW_(exception)     throw exception
	#define OEL_TRY_                  try
	#define OEL_CATCH_ALL             catch (...)
	#define OEL_WHEN_EXCEPTIONS_ON(x) x
#else
	#define OEL_THROW_(exception)     std::terminate()
	#define OEL_TRY_
	#define OEL_CATCH_ALL             OEL_CONST_COND if (false)
	#define OEL_WHEN_EXCEPTIONS_ON(x)
#endif

#if OEL_MEM_BOUND_DEBUG_LVL
	#define OEL_ASSERT_MEM_BOUND  ALWAYS_ASSERT_NOEXCEPT
#else
	#define OEL_ASSERT_MEM_BOUND(expr) ((void) 0)
#endif
#if !defined(NDEBUG)
	#define OEL_ASSERT  ALWAYS_ASSERT_NOEXCEPT
#else
	#define OEL_ASSERT(expr) ((void) 0)
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
	T * to_pointer_contiguous(__gnu_cxx::__normal_iterator<T *, U> it) noexcept { return it.base(); }
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
}

} // namespace oel

/// @cond FALSE
template<typename IteratorDest, typename IteratorSource>
struct oel::can_memmove_with : decltype( _detail::CanMemmoveWith(std::declval<IteratorDest>(),
																 std::declval<IteratorSource>()) ) {};
/// @endcond

template<typename T>
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};
