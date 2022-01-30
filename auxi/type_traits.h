#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../user_traits.h"

#include <iterator>


#if !__has_include(<boost/config.hpp>)
	#define OEL_NO_BOOST  1
#endif

namespace oel
{
namespace _detail
{
	template< typename T >
	typename T::is_always_equal IsAlwaysEqual(int);

	template< typename T >
	std::is_empty<T> IsAlwaysEqual(long);



	template< typename Iter >
	typename std::iterator_traits<Iter>::iterator_category IterCat(int);

	template< typename > void IterCat(long);


	template< typename Iter >
	typename Iter::iterator_concept IterConcept(int);

	template< typename > void IterConcept(long);


	template< typename Iter, typename Tag >
	constexpr bool IterCatIs()
	{
		return std::is_base_of< Tag, decltype(_detail::IterCat<Iter>(0)) >::value
		    or std::is_base_of< Tag, decltype(_detail::IterConcept<Iter>(0)) >::value;
	}
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


//! Return type of begin function (found by ADL)
template< typename Range >
using iterator_t = decltype( begin(std::declval<Range &>()) );
//! Return type of end function (found by ADL)
template< typename Range >
using sentinel_t = decltype( end(std::declval<Range &>()) );

#if __cpp_lib_concepts < 201907
	template< typename Iterator >
	using iter_difference_t = typename std::iterator_traits<Iterator>::difference_type;
#else
	using std::iter_difference_t;
#endif

#if __cpp_lib_concepts < 201907
	template< typename Iterator >
	using iter_value_t = typename std::iterator_traits<Iterator>::value_type;
#else
	using std::iter_value_t;
#endif

template< typename Iterator >
using iter_is_forward = bool_constant<
		_detail::IterCatIs<Iterator, std::forward_iterator_tag>()
		and std::is_copy_constructible<Iterator>::value
	>;
template< typename Iterator >
using iter_is_random_access = bool_constant<
		_detail::IterCatIs<Iterator, std::random_access_iterator_tag>()
		and std::is_copy_constructible<Iterator>::value
	>;
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



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename B0, typename B1 >
	constexpr bool conjunctionV = std::conditional_t<B0::value, B1, B0>::value;
}

} // namespace oel


template< typename T >
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};
