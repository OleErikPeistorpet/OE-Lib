#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#include <iterator>
#include <ranges>

#if __cpp_lib_ranges < 202202
	#define OEL_HAS_STD_ADAPTOR_CLOSURE  0
#else
	#define OEL_HAS_STD_ADAPTOR_CLOSURE  1
#endif

#if defined(_LIBCPP_VERSION) and _LIBCPP_VERSION < 15000
	#define OEL_HAS_STD_MOVE_SENTINEL  0
#else
	#define OEL_HAS_STD_MOVE_SENTINEL  1
#endif

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

namespace ranges = std::ranges;

using ranges::iterator_t;
using ranges::sentinel_t;


//! May fail for std::output_iterator_tag
template< typename Iterator, typename Tag >
inline constexpr bool iter_is  =  std::is_base_of_v< Tag, decltype( _detail::CheckedIterTag<Iterator>() ) >;

template< typename Iterator >
inline constexpr bool iter_is_random_access = iter_is<Iterator, std::random_access_iterator_tag>;


template< typename Sentinel >
struct _sentinelWrapper { Sentinel se; };

} // oel


#if !OEL_HAS_STD_MOVE_SENTINEL

// Small hack to let std::move_iterator< sentinel_wrapper<S> > compile
template< typename S >
struct std::iterator_traits< oel::_sentinelWrapper<S> >
 :	std::iterator_traits<S> {};

#endif
