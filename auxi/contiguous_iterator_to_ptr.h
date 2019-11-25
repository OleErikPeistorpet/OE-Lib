#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"

#include <memory> // for pointer_traits


namespace oel
{
namespace _detail
{
	// Part of pointer_traits for C++17
	template< typename PtrLike >
	OEL_ALWAYS_INLINE constexpr typename std::pointer_traits<PtrLike>::element_type * ToAddress(PtrLike p)
	{
		return p.operator->();
	}

	template< typename T >
	OEL_ALWAYS_INLINE constexpr T * ToAddress(T * p) { return p; }
}


//! If an IteratorSource range can be copied to an IteratorDest range with memmove, is-a true_type, else false_type
template< typename IteratorDest, typename IteratorSource >
struct can_memmove_with;


//! Convert iterator to pointer. This should be overloaded for each class of contiguous iterator (C++17 concept)
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
	#if _MSVC_STL_UPDATE < 201805L
	#define OEL_UNWRAP(iter)  _Unchecked(iter)
	#else
	#define OEL_UNWRAP(iter)  iter._Unwrapped()
	#endif
	template
	<	typename ContiguousIterator,
		enable_if< std::is_same<decltype( OEL_UNWRAP(ContiguousIterator{}) ),
		                        typename ContiguousIterator::pointer> ::value > = 0
	>
	constexpr auto to_pointer_contiguous(const ContiguousIterator & it) noexcept
	{
		return _detail::ToAddress(OEL_UNWRAP(it));
	}
	#undef OEL_UNWRAP
#endif

template< typename Iterator >
auto to_pointer_contiguous(std::move_iterator<Iterator> it) noexcept
->	decltype( to_pointer_contiguous(it.base()) )
	 { return to_pointer_contiguous(it.base()); }



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename T >
	is_trivially_copyable<T> CanMemmoveArrays(T * /*dest*/, const T *);

	template< typename IteratorDest, typename IterSource >
	auto CanMemmoveWith(IteratorDest dest, IterSource src)
	->	decltype( _detail::CanMemmoveArrays(to_pointer_contiguous(dest), to_pointer_contiguous(src)) );

	// SFINAE fallback for cases where to_pointer_contiguous(iterator) would be ill-formed or return types are not compatible
	false_type CanMemmoveWith(...);
}

} // namespace oel

//! @cond FALSE
template< typename IteratorDest, typename IteratorSource >
struct oel::can_memmove_with :
	decltype( _detail::CanMemmoveWith(std::declval<IteratorDest>(),
	                                  std::declval<IteratorSource>()) ) {};
//! @endcond
