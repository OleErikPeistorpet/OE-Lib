#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../user_traits.h"

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


//! Type returned by begin function (found by ADL)
template< typename Range >
using iterator_t = decltype( begin(std::declval<Range &>()) );

template< typename Iterator >
using iter_difference_t = typename std::iterator_traits<Iterator>::difference_type;

template< typename Iterator >
using iter_value_t = typename std::iterator_traits<Iterator>::value_type;

template< typename Iterator >
using iter_category = typename std::iterator_traits<Iterator>::iterator_category;

template< typename Iterator >
using iter_is_random_access = std::is_base_of< std::random_access_iterator_tag, iter_category<Iterator> >;

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


//! Same as std::enable_if_t<Condition, int>. Type int is intended as unused dummy
template< bool Condition >
using enable_if = typename std::enable_if<Condition, int>::type;


template< typename T >
using remove_cref_t = typename std::remove_const< typename std::remove_reference<T>::type >::type;

template< typename... Ts >
using common_type = typename std::common_type<Ts...>::type;



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename Alloc, typename Arg >
	decltype( std::declval<Alloc &>().construct( std::declval<typename Alloc::value_type *>(), std::declval<Arg>() ),
		true_type() )
		HasConstructTest(int);

	template< typename, typename >
	false_type HasConstructTest(long);

	template< typename Alloc, typename Arg >
	struct AllocHasConstruct : decltype( HasConstructTest<Alloc, Arg>(0) ) {};
}

} // namespace oel


template< typename T >
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};
