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

	template< typename > void IterCat(long);


	template< typename Iter >
	typename Iter::iterator_concept IterConcept(int);

	template< typename > void IterConcept(long);


	template< typename Iter, typename Tag >
	constexpr bool IterIs()
	{
		if constexpr (!std::is_copy_constructible_v<Iter>)
			return false;

		return std::is_base_of_v< Tag, decltype(_detail::IterCat<Iter>(0)) >
		    or std::is_base_of_v< Tag, decltype(_detail::IterConcept<Iter>(0)) >;
	}
}

////////////////////////////////////////////////////////////////////////////////

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
inline constexpr bool iter_is_forward       = _detail::IterIs<Iterator, std::forward_iterator_tag>();

template< typename Iterator >
inline constexpr bool iter_is_bidirectional = _detail::IterIs<Iterator, std::bidirectional_iterator_tag>();

template< typename Iterator >
inline constexpr bool iter_is_random_access = _detail::IterIs<Iterator, std::random_access_iterator_tag>();

/** @brief Partial emulation of std::sized_sentinel_for (C++20)
*
* Let i be an Iterator and s a Sentinel. If `s - i` is well-formed, then this value specifies whether
* that subtraction is invalid or not O(1). Must be specialized for some iterator, sentinel pairs. */
template< typename Sentinel, typename Iterator >
inline constexpr bool disable_sized_sentinel_for =
	#if __cpp_lib_concepts >= 201907
		std::disable_sized_sentinel_for<Sentinel, Iterator>;
	#else
		!iter_is_random_access<Iterator>;
	#endif

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template< typename Range >
	constexpr auto Size(Range && r)
	->	decltype(r.size()) { return r.size(); }

	template< typename Range, typename... None,
		enable_if<
			!disable_sized_sentinel_for< sentinel_t<Range>, iterator_t<Range> >
		> = 0
	>
	constexpr auto Size(Range && r, None...)
	->	decltype(end(r) - begin(r))
	{	return   end(r) - begin(r); }
}


template< typename Range, typename = void >
inline constexpr bool range_is_sized = false;

template< typename Range >
inline constexpr bool range_is_sized
	<	Range,
		std::void_t< decltype( _detail::Size(std::declval<Range &>()) ) >
	>	= true;



template< typename Sentinel >
struct sentinel_wrapper   { Sentinel _s; };

} // oel


#if !OEL_HAS_STD_MOVE_SENTINEL

// Small hack to let std::move_iterator< sentinel_wrapper<S> > compile
template< typename S >
struct std::iterator_traits< oel::sentinel_wrapper<S> >
 :	std::iterator_traits<S> {};

#endif
