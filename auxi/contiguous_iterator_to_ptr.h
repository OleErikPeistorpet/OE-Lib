#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#include <memory> // for pointer_traits

/** @file
*/

namespace oel
{

#if __cpp_lib_concepts >= 201907
	constexpr auto to_pointer_contiguous(std::contiguous_iterator auto it) noexcept
	{
		return std::to_address(it);
	}
#else
	namespace _detail
	{
		template< typename PtrLike >
		OEL_ALWAYS_INLINE constexpr typename std::pointer_traits<PtrLike>::element_type * ToAddress(PtrLike p)
		{
			return p.operator->();
		}

		template< typename T >
		OEL_ALWAYS_INLINE constexpr T * ToAddress(T * p) { return p; }
	}


	//! Convert iterator to pointer. This should be overloaded for each class of LegacyContiguousIterator (see cppreference)
	template< typename T >
	constexpr T * to_pointer_contiguous(T * it) noexcept { return it; }

	#ifdef __GLIBCXX__
		template< typename Ptr, typename C >
		constexpr typename std::pointer_traits<Ptr>::element_type *
			to_pointer_contiguous(__gnu_cxx::__normal_iterator<Ptr, C> it) noexcept
		{
			return _detail::ToAddress(it.base());
		}
	#elif _LIBCPP_VERSION
		template< typename T >
		constexpr T * to_pointer_contiguous(std::__wrap_iter<T *> it) noexcept { return it.base(); }

	#elif _CPPLIB_VER
		template< typename ContiguousIterator,
			enable_if<
				std::is_same_v< decltype(ContiguousIterator{}._Unwrapped()),
				                typename ContiguousIterator::pointer >
			> = 0
		>
		constexpr auto to_pointer_contiguous(const ContiguousIterator & it) noexcept
		{
			return _detail::ToAddress(it._Unwrapped());
		}
	#endif
#endif

template< typename Iterator >
constexpr auto to_pointer_contiguous(std::move_iterator<Iterator> it) noexcept
->	decltype( to_pointer_contiguous(it.base()) )
	 { return to_pointer_contiguous(it.base()); }

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template< typename T >
	std::is_trivially_copyable<T> CanMemmoveArrays(T * /*dest*/, const T *);

	template< typename IteratorDest, typename IterSource >
	auto CanMemmoveWith(IteratorDest dest, IterSource src)
	->	decltype( _detail::CanMemmoveArrays(to_pointer_contiguous(dest), to_pointer_contiguous(src)) );

	// SFINAE fallback for cases where to_pointer_contiguous(iterator) would be ill-formed or return types are not compatible
	false_type CanMemmoveWith(...);
}


//! Is true if an IteratorSource range can be copied to an IteratorDest range with memmove
template< typename IteratorDest, typename IteratorSource >
inline constexpr bool can_memmove_with =
	decltype(
		_detail::CanMemmoveWith(std::declval<IteratorDest>(),
		                        std::declval<IteratorSource>())
	)::value;

} // namespace oel
