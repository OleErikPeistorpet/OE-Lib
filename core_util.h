#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "user_traits.h"

#ifndef OEL_NO_BOOST
	#include <boost/iterator/iterator_categories.hpp>
#endif
#include <iterator>


/** @file
* @brief Basic utilities, included throughout the library
*
* Contains ssize, adl_begin, adl_end and more.
*/

#if !defined(NDEBUG) && !defined(OEL_MEM_BOUND_DEBUG_LVL)
/** @brief Undefined/0: no array index and iterator checks. 1: most debug checks. 2: all checks, often slow.
*
* 0 (or undefined by NDEBUG defined) is not binary compatible with levels 1 and 2. */
	#define OEL_MEM_BOUND_DEBUG_LVL 2
#endif

#if defined(NDEBUG) && OEL_MEM_BOUND_DEBUG_LVL == 0
	#define OEL_NOEXCEPT_NDEBUG noexcept
#else
	#define OEL_NOEXCEPT_NDEBUG
#endif


#if _MSC_VER
	#define OEL_CONST_COND __pragma(warning(suppress : 4127))
#else
	#define OEL_CONST_COND
#endif


#ifndef OEL_HALT
	/// Could throw an exception instead. Or write to log with __FILE__ and __LINE__, show a message, then abort.
	#if _MSC_VER
		#define OEL_HALT(failedCond) __debugbreak()
	#else
		#define OEL_HALT(failedCond) __asm__("int $3")
	#endif
#endif

#ifndef OEL_ALWAYS_ASSERT
	/// Just breaks into debugger on failure. Could be defined to standard from \c <cassert>
	#define OEL_ALWAYS_ASSERT(expr)  \
		OEL_CONST_COND  \
		do {  \
			if (!(expr)) OEL_HALT(#expr);  \
		} while (false)
#endif


namespace oel
{

using std::size_t;


using std::begin;
using std::end;

#if __cplusplus >= 201402L || _MSC_VER
	using std::cbegin;   using std::cend;
	using std::crbegin;  using std::crend;
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
constexpr std::ptrdiff_t ssize(const T(&)[Size]) noexcept  { return Size; }



/// Convert iterator to pointer. This should be overloaded for each class of contiguous iterator (C++17 concept)
template<typename T> inline
T * to_pointer_contiguous(T * ptr) noexcept  { return ptr; }

template<typename Iterator> inline
auto to_pointer_contiguous(std::move_iterator<Iterator> it) noexcept
 -> decltype( to_pointer_contiguous(it.base()) )  { return to_pointer_contiguous(it.base()); }


/// If an IteratorSource range can be copied to an IteratorDest range with memmove, is-a true_type, else false_type
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



/// Same as std::enable_if_t<Condition, int>. Type int is intended as unused dummy
template<bool Condition>
using enable_if = typename std::enable_if<Condition, int>::type;



using std::iterator_traits;

#ifndef OEL_NO_BOOST
	template<typename Iterator>
	using iterator_traversal_t = typename boost::iterator_traversal<Iterator>::type;

	using boost::single_pass_traversal_tag;
	using boost::forward_traversal_tag;
	using boost::random_access_traversal_tag;
#else
	template<typename Iterator>
	using iterator_traversal_t = typename iterator_traits<Iterator>::iterator_category;

	using single_pass_traversal_tag = std::input_iterator_tag;
	using forward_traversal_tag = std::forward_iterator_tag;
	using random_access_traversal_tag = std::random_access_iterator_tag;
#endif



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


#if defined(_CPPUNWIND) || defined(__EXCEPTIONS)
	#define OEL_THROW(exception)      throw exception
	#define OEL_TRY_                  try
	#define OEL_CATCH_ALL             catch (...)
	#define OEL_WHEN_EXCEPTIONS_ON(x) x
#else
	#define OEL_THROW(exception)      std::terminate()
	#define OEL_TRY_
	#define OEL_CATCH_ALL             OEL_CONST_COND if (false)
	#define OEL_WHEN_EXCEPTIONS_ON(x)
#endif

#if OEL_MEM_BOUND_DEBUG_LVL
	#define OEL_ASSERT_MEM_BOUND  OEL_ALWAYS_ASSERT
#else
	#define OEL_ASSERT_MEM_BOUND(expr) ((void) 0)
#endif
#if !defined(NDEBUG)
	#define OEL_ASSERT  OEL_ALWAYS_ASSERT
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
	false_type CanMemmoveWith(...);
}

} // namespace oel

/// @cond FALSE
template<typename IteratorDest, typename IteratorSource>
struct oel::can_memmove_with : decltype( _detail::CanMemmoveWith(std::declval<IteratorDest>(),
																 std::declval<IteratorSource>()) ) {};
/// @endcond

template<typename T>
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};
