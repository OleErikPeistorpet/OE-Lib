#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../fwd.h"

#include <iterator>


#ifdef __has_include
	#if !__has_include(<boost/config.hpp>)
	#define OEL_NO_BOOST  1
	#endif
#endif

namespace oel
{
namespace _detail
{
	template< typename T >
	typename T::is_always_equal IsAlwaysEqual(int);

	template< typename T >
	std::is_empty<T> IsAlwaysEqual(long);
}


//! Part of std::allocator_traits for C++17
template< typename T >
using is_always_equal   = decltype( _detail::IsAlwaysEqual<T>(0) );


template< bool... > struct bool_pack_t;

/** @brief Similar to std::conjunction, but is not short-circuiting
*
* Example: @code
template< typename... Ts >
struct Numbers {
	static_assert(oel::all_< std::is_arithmetic<Ts>... >::value, "Only arithmetic types, please");
@endcode  */
template< typename... BoolConstants >
struct all_   : std::is_same< bool_pack_t<true, BoolConstants::value...>,
                              bool_pack_t<BoolConstants::value..., true> > {};


using std::ptrdiff_t;
using std::size_t;


using std::begin;  using std::end;


template< typename Iterator >
using iter_difference_t = typename std::iterator_traits<Iterator>::difference_type;

template< typename Iterator >
using iter_value_t = typename std::iterator_traits<Iterator>::value_type;

template< typename Iterator >
using iter_category = typename std::iterator_traits<Iterator>::iterator_category;

template< typename Iterator >
using iter_is_random_access = std::is_base_of< std::random_access_iterator_tag, iter_category<Iterator> >;

template< typename Iterator >
using iter_is_forward = std::is_base_of< std::forward_iterator_tag, iter_category<Iterator> >;

/**
* @brief Partial emulation of std::sized_sentinel_for (C++20)
*
* Let i be an Iterator and s a Sentinel. If `s - i` is well-formed, then this type trait specifies
* whether that subtraction is valid and O(1). Must be specialized for some iterator, sentinel pairs. */
template< typename Sentinel, typename Iterator >
struct maybe_sized_sentinel_for
 :	bool_constant<
		iter_is_random_access<Iterator>::value or
		iter_is_random_access<Sentinel>::value
	> {};

//! Return type of begin function (found by ADL)
template< typename Range >
using iterator_t = decltype( begin(std::declval<Range &>()) );

template< typename Range >
using range_difference_t = iter_difference_t< iterator_t<Range> >;


//! Same as std::enable_if_t<Condition, int>. Type int is intended as unused dummy
template< bool Condition >
using enable_if = typename std::enable_if<Condition, int>::type;



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename B0, typename B1 >
	constexpr bool conjunctionV = std::conditional_t<B0::value, B1, B0>::value;
}

} // namespace oel


template< typename T >
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};


//! @cond INTERNAL

#if defined __cpp_deduction_guides or (_MSC_VER >= 1914 and _HAS_CXX17)
	#define OEL_HAS_DEDUCTION_GUIDES  1
#endif

#if __cpp_lib_concepts >= 201907
	#define OEL_REQUIRES(...) requires(__VA_ARGS__)
#else
	#define OEL_REQUIRES(...)
#endif


#ifdef __GNUC__
	#define OEL_ALWAYS_INLINE __attribute__((always_inline))
#else
	#define OEL_ALWAYS_INLINE
#endif

#ifdef _MSC_VER
	#define OEL_CONST_COND __pragma(warning(suppress : 4127 6326))
#else
	#define OEL_CONST_COND
#endif


#if defined _CPPUNWIND or defined __EXCEPTIONS
	#define OEL_THROW(exception, msg) throw exception
	#define OEL_TRY_                  try
	#define OEL_CATCH_ALL             catch (...)
	#define OEL_WHEN_EXCEPTIONS_ON(x) x
#else
	#define OEL_THROW(exc, message)   OEL_ABORT(message)
	#define OEL_TRY_
	#define OEL_CATCH_ALL             OEL_CONST_COND if (false)
	#define OEL_WHEN_EXCEPTIONS_ON(x)
#endif

//! @endcond
