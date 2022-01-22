#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../fwd.h"

#include <iterator>

//! Users can define (to override __cpp_lib_ranges or to not pay for include)
#ifndef OEL_STD_RANGES
	#if __cpp_lib_ranges < 201911
	#define OEL_STD_RANGES  0
	#else
	#define OEL_STD_RANGES  1
	#endif
#endif
#if OEL_STD_RANGES
#include <ranges>
#endif

#if !__has_include(<boost/config.hpp>)
	#define OEL_NO_BOOST  1
#endif

namespace oel
{
namespace _detail
{
	template< typename Alloc >  // pass dummy int to prefer this overload
	bool_constant<Alloc::can_reallocate()> CanRealloc(int);

	template< typename >
	false_type CanRealloc(long);


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

template< typename Alloc >
struct allocator_can_realloc   : decltype( _detail::CanRealloc<Alloc>(0) ) {};

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

//! Like std::ranges::borrowed_iterator_t, but doesn't require that Range has end()
template< typename Range >
using borrowed_iterator_t =
#if OEL_STD_RANGES
	std::conditional_t<
		std::is_lvalue_reference_v<Range> or std::ranges::enable_borrowed_range< std::remove_cvref_t<Range> >,
		iterator_t<Range>,
		std::ranges::dangling
	>;
#else
	iterator_t<Range>;
#endif

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
constexpr bool iter_is_forward =
	_detail::IterCatIs<Iterator, std::forward_iterator_tag>()
	and std::is_copy_constructible<Iterator>::value;

template< typename Iterator >
constexpr bool iter_is_random_access =
	_detail::IterCatIs<Iterator, std::random_access_iterator_tag>()
	and std::is_copy_constructible<Iterator>::value;

/**
* @brief Partial emulation of std::sized_sentinel_for (C++20)
*
* Let i be an Iterator and s a Sentinel. If `s - i` is well-formed, then this value specifies whether
* that subtraction is invalid or not O(1). Must be specialized for some iterator, sentinel pairs. */
template< typename Sentinel, typename Iterator >
constexpr bool disable_sized_sentinel_for =
	#if __cpp_lib_concepts >= 201907
		std::disable_sized_sentinel_for<Sentinel, Iterator>;
	#else
		!(iter_is_random_access<Sentinel> or iter_is_random_access<Iterator>);
	#endif


//! Same as std::enable_if_t<Condition, int>. Type int is intended as unused dummy
template< bool Condition >
using enable_if = typename std::enable_if<Condition, int>::type;



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename B0, typename B1 >
	constexpr bool conjunctionV = std::conditional_t<B0::value, B1, B0>::value;


	template< typename > constexpr bool isUnboundedArray = false;

	template< typename T >
	constexpr bool isUnboundedArray<T[]> = true;
}

} // namespace oel


template< typename T >
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};


//! @cond INTERNAL

#if __cpp_deduction_guides or (_MSC_VER >= 1914 and _HAS_CXX17)
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
	#define OEL_HAS_EXCEPTIONS        1
	#define OEL_THROW(exception, msg) throw exception
	#define OEL_RETHROW               throw
	#define OEL_TRY_                  try
	#define OEL_CATCH_ALL             catch (...)
#else
	#define OEL_HAS_EXCEPTIONS        0
	#define OEL_THROW(exc, message)   OEL_ABORT(message)
	#define OEL_RETHROW
	#define OEL_TRY_
	#define OEL_CATCH_ALL             OEL_CONST_COND if (false)
#endif

//! @endcond
