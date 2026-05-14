#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

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
#include <iterator>

/** @file
*/

namespace oel
{
namespace _detail
{
	template< typename Iter >
	typename std::iterator_traits<Iter>::iterator_category IterCat(int);

	template< typename > std::input_iterator_tag IterCat(long);


	template< typename Iter >
	typename Iter::iterator_concept IterConcept(int);

	template< typename > void IterConcept(long);


	template< typename Iter >
	constexpr auto CheckedIterTag()
	{
		if constexpr( std::is_copy_constructible_v<Iter> )
		{
			using Cat     = decltype( _detail::IterCat<Iter>(0) );
			using Concept = decltype( _detail::IterConcept<Iter>(0) );
			if constexpr( std::is_base_of_v<Cat, Concept> )
				return Concept{};
			else
				return Cat{};
		}
		else
		{	return std::input_iterator_tag{};
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

//! Like std::ranges::begin, but for C++17
template< typename Range >  OEL_ALWAYS_INLINE
constexpr auto begin_(Range && r) -> decltype( std::begin(r) )  { return std::begin(r); }

template< typename Range >  OEL_ALWAYS_INLINE
constexpr auto end_(Range && r)   -> decltype( std::end(r) )  { return std::end(r); }

template< typename Range, typename... None >
constexpr auto begin_(Range && r, None...) -> decltype( begin(r) )  { return begin(r); }

template< typename Range, typename... None >
constexpr auto end_(Range && r, None...)   -> decltype( end(r) )  { return end(r); }


//! Return type of begin function (maybe found by ADL)
template< typename Range >
using iterator_t = decltype( oel::begin_(std::declval<Range &>()) );
//! Return type of end function
template< typename Range >
using sentinel_t = decltype( oel::end_(std::declval<Range &>()) );

#if __cpp_lib_concepts < 201907
	template< typename Iterator >
	using iter_difference_t = typename std::iterator_traits<Iterator>::difference_type;

	template< typename Iterator >
	using iter_value_t = typename std::iterator_traits<Iterator>::value_type;
#else
	using std::iter_difference_t;
	using std::iter_value_t;
#endif

//! May fail for std::output_iterator_tag
template< typename Iterator, typename Tag >
inline constexpr bool iter_is  =  std::is_base_of_v< Tag, decltype( _detail::CheckedIterTag<Iterator>() ) >;

template< typename Iterator >
inline constexpr bool iter_is_random_access = iter_is<Iterator, std::random_access_iterator_tag>;

//! Partial emulation of std::sized_sentinel_for (C++20)
/**
* Let i be an Iterator and s a Sentinel. If `s - i` is well-formed, then this value specifies whether
* that subtraction is invalid or not O(1). Must be specialized for some iterator, sentinel pairs. */
#if __cpp_lib_concepts < 201907
	template< typename Sentinel, typename Iterator >
	inline constexpr bool disable_sized_sentinel_for = !iter_is_random_access<Iterator>;
#else
	using std::disable_sized_sentinel_for;
#endif

#if OEL_STD_RANGES
	using std::ranges::enable_view;
#else
	template< typename >
	inline constexpr bool enable_view = false;
#endif

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template< typename T, size_t N >
	constexpr size_t Size(T(&)[N]) noexcept { return N; }

	template< typename Range >
	constexpr auto Size(Range && r) noexcept(noexcept( r.size() ))
	->	decltype( r.size() ) { return r.size(); }

	template
	<	typename Range, typename... None,
		enable_if<
			!disable_sized_sentinel_for< sentinel_t<Range>, iterator_t<Range> >
		> = 0
	>
	constexpr auto Size(Range && r, None...)
	->	decltype( oel::end_(r) - oel::begin_(r) )
	{	return    oel::end_(r) - oel::begin_(r); }
}


template< typename Range, typename = void >
inline constexpr bool range_is_sized = false;

template< typename Range >
inline constexpr bool range_is_sized
	<	Range,
		std::void_t< decltype( _detail::Size(std::declval<Range &>()) ) >
	>	= true;



template< typename Sentinel >
struct sentinel_wrapper { Sentinel se; };

} // oel


#if !OEL_HAS_STD_MOVE_SENTINEL

// Small hack to let std::move_iterator< sentinel_wrapper<S> > compile
template< typename S >
struct std::iterator_traits< oel::sentinel_wrapper<S> >
 :	std::iterator_traits<S> {};

#endif
