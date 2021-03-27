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
namespace _detail
{
	template< typename RandomAccessIter >
	struct SentinelAt
	{
		template< typename I = RandomAccessIter, enable_if< iter_is_random_access<I> > = 0 >
		static constexpr I call(I it, iter_difference_t<I> n)
		{
			return it + n;
		}
	};



	template< typename T,
	          bool = std::is_move_assignable<T>::value >
	class AssignableWrap
	{
		static_assert( std::is_trivially_copy_constructible<T>::value and std::is_trivially_destructible<T>::value,
			"Transform function must be move assignable, or trivially copy constructible and trivially destructible" );

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

}