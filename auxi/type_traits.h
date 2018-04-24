#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../user_traits.h"

#ifndef OEL_NO_BOOST
#include <boost/iterator/iterator_categories.hpp>
#endif
#include <iterator>


namespace oel
{

//! Same as std::enable_if_t<Condition, int>. Type int is intended as unused dummy
template<bool Condition>
using enable_if = typename std::enable_if<Condition, int>::type;



template<bool...> struct bool_pack_t;

//! If all of Vs are equal to true, this is-a true_type, else false_type
template<bool... Vs>
using all_true   = std::is_same< bool_pack_t<true, Vs...>, bool_pack_t<Vs..., true> >;

/** @brief Similar to std::conjunction, but is not short-circuiting
*
* Example: @code
template<typename... Ts>
void ProcessNumbers(Ts... n) {
	static_assert(oel::all_< std::is_arithmetic<Ts>... >::value, "Only arithmetic types, please");
@endcode  */
template<typename... BoolConstants>
struct all_ : all_true<BoolConstants::value...> {};



//! If an IteratorSource range can be copied to an IteratorDest range with memmove, is-a true_type, else false_type
template<typename IteratorDest, typename IteratorSource>
struct can_memmove_with;


//! Range::difference_type if present, else std::ptrdiff_t
template<typename Range>
using range_difference_t  = decltype( _detail::DiffT<Range>(0) );

template<typename Iterator>
using iterator_difference_t = typename std::iterator_traits<Iterator>::difference_type;

template<typename Iterator>
using iterator_traversal_t
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
using iterator_is_random_access = std::is_base_of< random_access_traversal_tag, iterator_traversal_t<Iterator> >;



//! Part of std::allocator_traits for C++17
template<typename T>
using is_always_equal  = decltype( _detail::IsAlwaysEqual<T>(0) );



#if defined __GLIBCXX__ and __GNUC__ == 4
	template<typename T>
	using is_trivially_default_constructible = bool_constant< __has_trivial_constructor(T)
		#ifdef __INTEL_COMPILER
			or __is_pod(T)
		#endif
		>;
#else
	using std::is_trivially_default_constructible;
#endif


using std::size_t;



////////////////////////////////////////////////////////////////////////////////



template<typename T>
T * to_pointer_contiguous(T *) noexcept;

namespace _detail
{
	template<typename T>
	is_trivially_copyable<T> CanMemmoveArrays(T * /*dest*/, const T *);

	template<typename IterDest, typename IterSrc>
	auto CanMemmoveWith(IterDest dest, IterSrc src)
	 -> decltype( _detail::CanMemmoveArrays(to_pointer_contiguous(dest), to_pointer_contiguous(src)) );

	// SFINAE fallback for cases where to_pointer_contiguous(iterator) would be ill-formed or return types are not compatible
	false_type CanMemmoveWith(...);
}

} // namespace oel

//! @cond FALSE
template<typename IteratorDest, typename IteratorSource>
struct oel::can_memmove_with :
	decltype( _detail::CanMemmoveWith(std::declval<IteratorDest>(),
	                                  std::declval<IteratorSource>()) ) {};
//! @endcond


template<typename T>
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};
