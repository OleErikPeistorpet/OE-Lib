#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "adl_begin_end.h"

#ifndef OEL_NO_BOOST
#include <boost/iterator/iterator_categories.hpp>
#endif


namespace oel
{
namespace _detail
{
	template<typename T>
	typename T::is_always_equal IsAlwaysEqual(int);

	template<typename T>
	std::is_empty<T> IsAlwaysEqual(long);
}


template<bool...> struct bool_pack_t;

/** @brief Similar to std::conjunction, but is not short-circuiting
*
* Example: @code
template<typename... Ts>
void ProcessNumbers(Ts... n) {
	static_assert(oel::all_< std::is_arithmetic<Ts>... >::value, "Only arithmetic types, please");
@endcode  */
template<typename... BoolConstants>
struct all_   : std::is_same< bool_pack_t<true, BoolConstants::value...>,
                              bool_pack_t<BoolConstants::value..., true> > {};


//! Part of std::allocator_traits for C++17
template<typename T>
using is_always_equal   = decltype( _detail::IsAlwaysEqual<T>(0) );


//! Type returned by begin function (found by ADL)
template<typename Range>
using iterator_t = decltype( begin(std::declval<Range &>()) );

template<typename Iterator>
using iter_difference_t = typename std::iterator_traits<Iterator>::difference_type;

template<typename Iterator>
using iter_value_t = typename std::iterator_traits<Iterator>::value_type;

template<typename Iterator>
using iter_traversal_t
#ifndef OEL_NO_BOOST
	= typename boost::iterator_traversal<Iterator>::type;

	using boost::single_pass_traversal_tag;
	using boost::forward_traversal_tag;
	using boost::random_access_traversal_tag;
#else
	= typename std::iterator_traits<Iterator>::iterator_category;

	using single_pass_traversal_tag = std::input_iterator_tag;
	using forward_traversal_tag = std::forward_iterator_tag;
	using random_access_traversal_tag = std::random_access_iterator_tag;
#endif

template<typename Iterator>
using iter_is_random_access = std::is_base_of< random_access_traversal_tag, iter_traversal_t<Iterator> >;



//! Same as std::enable_if_t<Condition, int>. Type int is intended as unused dummy
template<bool Condition>
using enable_if = typename std::enable_if<Condition, int>::type;


template<typename... Ts>
using common_type = typename std::common_type<Ts...>::type;



#if defined __GLIBCXX__ and __GNUC__ == 4
	template<typename T>
	using is_trivially_default_constructible = bool_constant<__has_trivial_constructor(T)>;
#else
	using std::is_trivially_default_constructible;
#endif



using std::size_t;



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template<typename Alloc, typename Arg>
	decltype( std::declval<Alloc &>().construct( (typename Alloc::value_type *)0, std::declval<Arg>() ),
		true_type() )
		HasConstructTest(int);

	template<typename, typename>
	false_type HasConstructTest(long);

	template<typename Alloc, typename Arg>
	using AllocHasConstruct = decltype( HasConstructTest<Alloc, Arg>(0) );
}

} // namespace oel


template<typename T>
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};
