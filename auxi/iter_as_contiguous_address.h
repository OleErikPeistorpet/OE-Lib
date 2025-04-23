#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#include <iterator>

/** @file
*/

namespace oel
{
namespace iter
{

#if __cpp_lib_concepts >= 201907

	constexpr auto as_contiguous_address(std::contiguous_iterator auto it) noexcept
	{
		return std::to_address(it);
	}
#else

	//! Convert iterator to pointer. This should be overloaded for each LegacyContiguousIterator class (see cppreference)
	template< typename T >
	constexpr T * as_contiguous_address(T * it) noexcept { return it; }

	#ifdef __GLIBCXX__

	template< typename T, typename C >
	constexpr T * as_contiguous_address(__gnu_cxx::__normal_iterator<T *, C> it) noexcept
		{
			return it.base();
		}
	#elif _LIBCPP_VERSION

	template< typename T >
	constexpr T * as_contiguous_address(std::__wrap_iter<T *> it) noexcept  { return it.base(); }

	#elif _CPPLIB_VER
	template
	<	typename ContiguousIterator,
		enable_if
		<	std::is_pointer_v<decltype( ContiguousIterator{}._Unwrapped() )>
		> = 0
	>
	constexpr auto as_contiguous_address(const ContiguousIterator & it) noexcept
		{
			return it._Unwrapped();
		}
	#endif
#endif

template< typename Iterator >
constexpr auto as_contiguous_address(std::move_iterator<Iterator> it)
	noexcept(noexcept( as_contiguous_address(it.base()) ))
->	decltype(          as_contiguous_address(it.base()) )
	{        return    as_contiguous_address(it.base()); }

} // iter

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	using iter::as_contiguous_address;


	template< typename T >
	std::is_trivially_copyable<T> CanMemmoveArrays(T * /*dest*/, const T *);


	template< typename IteratorDest, typename IterSource >
	auto CanMemmoveWith(IteratorDest dest, IterSource src)
	->	decltype( _detail::CanMemmoveArrays(as_contiguous_address(dest), as_contiguous_address(src)) );

	// SFINAE fallback for cases where as_contiguous_address(iterator) would be ill-formed or return types are not compatible
	false_type CanMemmoveWith(...);
}


//! Is true if an IteratorSource range can be copied to an IteratorDest range with memmove
template< typename IteratorDest, typename IteratorSource >
inline constexpr bool can_memmove_with =
	decltype
	(	_detail::CanMemmoveWith(std::declval<IteratorDest>(),
		                        std::declval<IteratorSource>())
	)::value;

}