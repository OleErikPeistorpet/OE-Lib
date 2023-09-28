#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../../auxi/type_traits.h"

//! Users may define (to override __cpp_lib_ranges or to not pay for include)
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

namespace oel
{

template< typename Sentinel >
struct sentinel_wrapper   { Sentinel _s; };



namespace _detail
{
	template< typename T,
		enable_if< std::is_copy_constructible<T>::value > = 0
	> OEL_ALWAYS_INLINE
	constexpr const T & MoveIfNotCopyable(T & ob) { return ob; }

	template< typename T, typename... None >
	constexpr T MoveIfNotCopyable(T & ob, None...)
	{
		return static_cast<T &&>(ob);
	}



	template< typename T,
	          bool = std::is_move_assignable<T>::value >
	class AssignableWrap
	{
		static_assert( std::is_trivially_copy_constructible<T>::value and std::is_trivially_destructible<T>::value,
			"The user-supplied function must be move assignable, or trivially copy constructible and trivially destructible" );

		union Impl
		{
			struct {} _none;
			T _val;

			constexpr Impl() noexcept : _none{} {}
			constexpr Impl(const T & src) noexcept : _val(src) {}

			Impl(const Impl &) = default;

			void operator =(const Impl & other) & noexcept
			{
				::new(this) Impl(other);
			}

			OEL_ALWAYS_INLINE constexpr operator const T &() const { return _val; }
			OEL_ALWAYS_INLINE constexpr operator       T &()       { return _val; }
		};

		using Empty_type_MSVC_unique_name = T;

		struct ImplEmpty : Empty_type_MSVC_unique_name
		{
			constexpr ImplEmpty(const Empty_type_MSVC_unique_name & src) noexcept
			 :	Empty_type_MSVC_unique_name(src) {}

			ImplEmpty() = default;
			ImplEmpty(const ImplEmpty &) = default;

			OEL_ALWAYS_INLINE constexpr void operator =(const ImplEmpty &) & noexcept {}
		};

	public:
		using Type = std::conditional_t< std::is_empty<T>::value, ImplEmpty, Impl >;
	};

	template< typename T >
	class AssignableWrap<T, true>
	{
	public:
		using Type = T;
	};
}

} // oel


#if !OEL_STD_RANGES
namespace std
{
// Small hack to let std::move_iterator< sentinel_wrapper<S> > compile
template< typename S >
struct iterator_traits< oel::sentinel_wrapper<S> > : iterator_traits<S> {};

}
#endif
